/* SPDX-License-Identifier: MIT */
#include "net/modrinth/mcpkg_modrinth_json.h"

#include "container/mcpkg_list.h"
#include "container/mcpkg_str_list.h"

#include "mp/mcpkg_mp_util.h"
#include "mp/mcpkg_mp_pkg_meta.h"
#include "mp/mcpkg_mp_pkg_file.h"
#include "mp/mcpkg_mp_pkg_digest.h"
#include "mp/mcpkg_mp_pkg_depends.h"
#include "mp/mcpkg_mp_pkg_origin.h"

#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* ---- helpers ---- */

static char *dup_str(const char *s)
{
	return mcpkg_mp_util_dup_str(s);
}

static int sl_push(struct McPkgStringList *sl, const char *s)
{
	return mcpkg_stringlist_push(sl, s);
}

static int str_ieq(const char *a, const char *b)
{
	if (!a || !b) return 0;
	for (; *a && *b; a++, b++) {
		unsigned char ca = (unsigned char) * a;
		unsigned char cb = (unsigned char) * b;
		if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
		if (ca != cb) return 0;
	}
	return *a == '\0' && *b == '\0';
}

static int contains_str_array_ci(const cJSON *arr, const char *needle)
{
	const cJSON *e;

	if (!cJSON_IsArray(arr) || !needle || !needle[0])
		return 0;

	cJSON_ArrayForEach(e, arr) {
		if (cJSON_IsString(e) && e->valuestring && str_ieq(e->valuestring, needle))
			return 1;
	}
	return 0;
}

static const cJSON *array_index(const cJSON *arr, int idx)
{
	int i = 0;
	const cJSON *e;

	if (!cJSON_IsArray(arr) || idx < 0)
		return NULL;

	cJSON_ArrayForEach(e, arr) {
		if (i == idx) return e;
		i++;
	}
	return NULL;
}

static int32_t side_str_to_flag(const char *s)
{
	if (!s || !*s) return -1;
	if (str_ieq(s, "unsupported")) return 0;
	return 1; /* required/optional -> YES */
}

static uint32_t dep_kind_from_str(const char *s)
{
	if (str_ieq(s, "required"))     return 0u;
	if (str_ieq(s, "optional"))     return 1u;
	if (str_ieq(s, "incompatible")) return 2u;
	if (str_ieq(s, "embedded"))     return 3u;
	return 0u;
}

/* list<char*> helpers */
static int push_string_ptr(struct McPkgList *lst, const char *s)
{
	char *d;

	if (!lst || !s || !s[0])
		return MCPKG_MODRINTH_JSON_ERR_INVALID;

	d = dup_str(s);
	if (!d)
		return MCPKG_MODRINTH_JSON_ERR_NOMEM;
	if (mcpkg_list_push(lst, &d) != MCPKG_CONTAINER_OK) {
		free(d);
		return MCPKG_MODRINTH_JSON_ERR_NOMEM;
	}
	return MCPKG_MODRINTH_JSON_NO_ERROR;
}

static void free_string_ptr_list(struct McPkgList *lst)
{
	size_t i, n;
	if (!lst) return;
	n = mcpkg_list_size(lst);
	for (i = 0; i < n; i++) {
		char *s = NULL;
		if (mcpkg_list_at(lst, i, &s) == MCPKG_CONTAINER_OK && s)
			free(s);
	}
	mcpkg_list_free(lst);
}

/* list<McPkgModrinthHit*> helper */
static void free_hit_list(struct McPkgList *lst)
{
	size_t i, n;
	if (!lst) return;
	n = mcpkg_list_size(lst);
	for (i = 0; i < n; i++) {
		struct McPkgModrinthHit *h = NULL;
		if (mcpkg_list_at(lst, i, &h) == MCPKG_CONTAINER_OK && h)
			mcpkg_modrinth_hit_free(h);
	}
	mcpkg_list_free(lst);
}

/* ---- public: hit struct ---- */

MCPKG_API void
mcpkg_modrinth_hit_free(struct McPkgModrinthHit *h)
{
	if (!h) return;
	if (h->project_id) free(h->project_id);
	if (h->slug) free(h->slug);
	if (h->title) free(h->title);
	if (h->description) free(h->description);
	if (h->license_id) free(h->license_id);
	if (h->icon_url) free(h->icon_url);
	if (h->categories) mcpkg_stringlist_free(h->categories);
	free(h);
}

/* ---- public: parse search (detailed) ---- */

MCPKG_API int
mcpkg_modrinth_parse_search_hits_detailed(const char *json,
                size_t json_len,
                struct McPkgList **out_hits,
                uint64_t *out_total_hits)
{
	struct McPkgList *hits_out = NULL;
	char *tmp = NULL;
	cJSON *root = NULL, *hits = NULL;
	uint64_t total = 0;
	int ret = MCPKG_MODRINTH_JSON_NO_ERROR;

	if (!json || !out_hits || !out_total_hits)
		return MCPKG_MODRINTH_JSON_ERR_INVALID;

	hits_out = mcpkg_list_new(sizeof(struct McPkgModrinthHit *), NULL, 0, 0);
	if (!hits_out)
		return MCPKG_MODRINTH_JSON_ERR_NOMEM;

	tmp = (char *)malloc(json_len + 1U);
	if (!tmp) {
		ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
		goto out;
	}
	memcpy(tmp, json, json_len);
	tmp[json_len] = '\0';

	root = cJSON_Parse(tmp);
	if (!root) {
		ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
		goto out;
	}

	{
		cJSON *tot = cJSON_GetObjectItemCaseSensitive(root, "total_hits");
		if (cJSON_IsNumber(tot) && tot->valuedouble >= 0.0)
			total = (uint64_t)tot->valuedouble;
	}

	hits = cJSON_GetObjectItemCaseSensitive(root, "hits");
	if (!cJSON_IsArray(hits)) {
		ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
		goto out;
	}

	{
		const cJSON *hit;
		uint64_t count = 0;

		cJSON_ArrayForEach(hit, hits) {
			struct McPkgModrinthHit *h = NULL;
			const cJSON *pid = cJSON_GetObjectItemCaseSensitive(hit, "project_id");
			const cJSON *slug = cJSON_GetObjectItemCaseSensitive(hit, "slug");
			const cJSON *title = cJSON_GetObjectItemCaseSensitive(hit, "title");
			const cJSON *desc = cJSON_GetObjectItemCaseSensitive(hit, "description");
			const cJSON *lic  = cJSON_GetObjectItemCaseSensitive(hit, "license");
			const cJSON *icon = cJSON_GetObjectItemCaseSensitive(hit, "icon_url");
			const cJSON *cli  = cJSON_GetObjectItemCaseSensitive(hit, "client_side");
			const cJSON *srv  = cJSON_GetObjectItemCaseSensitive(hit, "server_side");
			const cJSON *cats = cJSON_GetObjectItemCaseSensitive(hit, "categories");

			const char *id_str = (cJSON_IsString(pid)
			                      && pid->valuestring) ? pid->valuestring :
			                     (cJSON_IsString(slug) && slug->valuestring) ? slug->valuestring : NULL;
			if (!id_str || !id_str[0])
				continue;

			h = (struct McPkgModrinthHit *)calloc(1, sizeof(*h));
			if (!h) {
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out;
			}

			if (cJSON_IsString(pid)
			    && pid->valuestring)  h->project_id = dup_str(pid->valuestring);
			if (cJSON_IsString(slug)
			    && slug->valuestring) h->slug       = dup_str(slug->valuestring);
			if (cJSON_IsString(title)
			    && title->valuestring)h->title      = dup_str(title->valuestring);
			if (cJSON_IsString(desc)
			    && desc->valuestring)  h->description = dup_str(desc->valuestring);
			if (cJSON_IsString(lic)
			    && lic->valuestring)   h->license_id = dup_str(lic->valuestring);
			if (cJSON_IsString(icon)
			    && icon->valuestring)  h->icon_url   = dup_str(icon->valuestring);

			h->client = cJSON_IsString(cli)
			            && cli->valuestring ? side_str_to_flag(cli->valuestring) : -1;
			h->server = cJSON_IsString(srv)
			            && srv->valuestring ? side_str_to_flag(srv->valuestring) : -1;

			if (cJSON_IsArray(cats)) {
				const cJSON *e;
				h->categories = mcpkg_stringlist_new(0, 0);
				if (!h->categories) {
					mcpkg_modrinth_hit_free(h);
					ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
					goto out;
				}
				cJSON_ArrayForEach(e, cats) {
					if (cJSON_IsString(e) && e->valuestring && e->valuestring[0]) {
						if (sl_push(h->categories, e->valuestring) != 0) {
							mcpkg_modrinth_hit_free(h);
							ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
							goto out;
						}
					}
				}
			}

			if (mcpkg_list_push(hits_out, &h) != MCPKG_CONTAINER_OK) {
				mcpkg_modrinth_hit_free(h);
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out;
			}
			count++;
		}

		if (total == 0) total = count;
	}

	*out_hits = hits_out;
	*out_total_hits = total;
	hits_out = NULL;
	ret = MCPKG_MODRINTH_JSON_NO_ERROR;

out:
	if (hits_out) free_hit_list(hits_out);
	if (root) cJSON_Delete(root);
	if (tmp) free(tmp);
	return ret;
}

/* ---- public: parse search (ids only) ---- */

MCPKG_API int
mcpkg_modrinth_parse_search_hit_ids(const char *json,
                                    size_t json_len,
                                    struct McPkgList **out_ids,
                                    uint64_t *out_total_hits)
{
	struct McPkgList *ids = NULL;
	char *tmp = NULL;
	cJSON *root = NULL, *hits = NULL;
	uint64_t total = 0;
	int ret = MCPKG_MODRINTH_JSON_NO_ERROR;

	if (!json || !out_ids || !out_total_hits)
		return MCPKG_MODRINTH_JSON_ERR_INVALID;

	ids = mcpkg_list_new(sizeof(char *), NULL, 0, 0);
	if (!ids)
		return MCPKG_MODRINTH_JSON_ERR_NOMEM;

	tmp = (char *)malloc(json_len + 1U);
	if (!tmp) {
		ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
		goto out;
	}
	memcpy(tmp, json, json_len);
	tmp[json_len] = '\0';

	root = cJSON_Parse(tmp);
	if (!root) {
		ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
		goto out;
	}

	{
		cJSON *tot = cJSON_GetObjectItemCaseSensitive(root, "total_hits");
		if (cJSON_IsNumber(tot) && tot->valuedouble >= 0.0)
			total = (uint64_t)tot->valuedouble;
	}

	hits = cJSON_GetObjectItemCaseSensitive(root, "hits");
	if (!cJSON_IsArray(hits)) {
		ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
		goto out;
	}

	{
		const cJSON *hit;
		uint64_t count = 0;

		cJSON_ArrayForEach(hit, hits) {
			const cJSON *pid  = cJSON_GetObjectItemCaseSensitive(hit, "project_id");
			const cJSON *slug = cJSON_GetObjectItemCaseSensitive(hit, "slug");
			const char *id = (cJSON_IsString(pid)
			                  && pid->valuestring)  ? pid->valuestring :
			                 (cJSON_IsString(slug) && slug->valuestring) ? slug->valuestring : NULL;
			if (id && id[0]) {
				ret = push_string_ptr(ids, id);
				if (ret != MCPKG_MODRINTH_JSON_NO_ERROR) goto out;
				count++;
			}
		}

		if (total == 0) total = count;
	}

	*out_ids = ids;
	ids = NULL;
	*out_total_hits = total;
	ret = MCPKG_MODRINTH_JSON_NO_ERROR;

out:
	if (ids) free_string_ptr_list(ids);
	if (root) cJSON_Delete(root);
	if (tmp) free(tmp);
	return ret;
}

/* ---- public: choose best version index ---- */

MCPKG_API int
mcpkg_modrinth_choose_version_idx(const char *versions_json,
                                  size_t versions_len,
                                  const char *want_loader,
                                  const char *want_game_version,
                                  int featured_only,
                                  int *out_idx)
{
	char *tmp = NULL;
	cJSON *arr = NULL;
	const cJSON *ver;
	int idx = -1;
	int best_featured = -1;
	int best_listed = -1;
	const char *best_date = NULL;
	int i = 0;

	if (!versions_json || !out_idx)
		return MCPKG_MODRINTH_JSON_ERR_INVALID;

	tmp = (char *)malloc(versions_len + 1U);
	if (!tmp) return MCPKG_MODRINTH_JSON_ERR_NOMEM;
	memcpy(tmp, versions_json, versions_len);
	tmp[versions_len] = '\0';

	arr = cJSON_Parse(tmp);
	if (!cJSON_IsArray(arr)) {
		cJSON_Delete(arr);
		free(tmp);
		return MCPKG_MODRINTH_JSON_ERR_PARSE;
	}

	cJSON_ArrayForEach(ver, arr) {
		const cJSON *loaders = cJSON_GetObjectItemCaseSensitive(ver, "loaders");
		const cJSON *gvers   = cJSON_GetObjectItemCaseSensitive(ver, "game_versions");
		const cJSON *featured = cJSON_GetObjectItemCaseSensitive(ver, "featured");
		const cJSON *status  = cJSON_GetObjectItemCaseSensitive(ver, "status");
		const cJSON *date    = cJSON_GetObjectItemCaseSensitive(ver, "date_published");

		if (want_loader && *want_loader) {
			if (!contains_str_array_ci(loaders, want_loader)) {
				i++;
				continue;
			}
		}
		if (want_game_version && *want_game_version) {
			if (!contains_str_array_ci(gvers, want_game_version)) {
				i++;
				continue;
			}
		}
		if (featured_only) {
			if (!cJSON_IsBool(featured) || !cJSON_IsTrue(featured)) {
				i++;
				continue;
			}
		}

		{
			int v_featured = (cJSON_IsBool(featured) && cJSON_IsTrue(featured)) ? 1 : 0;
			int v_listed   = (cJSON_IsString(status)
			                  && str_ieq(status->valuestring, "listed")) ? 1 : 0;
			const char *v_date = (cJSON_IsString(date)
			                      && date->valuestring) ? date->valuestring : "";

			if (idx == -1) {
				idx = i;
				best_featured = v_featured;
				best_listed = v_listed;
				best_date = v_date;
			} else {
				if (v_featured != best_featured) {
					if (v_featured > best_featured) {
						idx = i;
						best_featured = v_featured;
						best_listed = v_listed;
						best_date = v_date;
					}
				} else if (v_listed != best_listed) {
					if (v_listed > best_listed) {
						idx = i;
						best_listed = v_listed;
						best_date = v_date;
					}
				} else {
					if (best_date == NULL || strcmp(v_date, best_date) > 0) {
						idx = i;
						best_date = v_date;
					}
				}
			}
		}
		i++;
	}

	*out_idx = idx;
	cJSON_Delete(arr);
	free(tmp);
	return MCPKG_MODRINTH_JSON_NO_ERROR;
}

/* ---- public: build McPkgCache from chosen version + optional hit ---- */

MCPKG_API int
mcpkg_modrinth_build_pkg_meta(const struct McPkgModrinthHit *hit,
                              const char *versions_json,
                              size_t versions_len,
                              int ver_idx,
                              const char *provider,
                              struct McPkgCache **out_cache,
                              uint32_t *out_flags)
{
	char *tmp = NULL;
	cJSON *arr = NULL;
	const cJSON *ver = NULL;
	struct McPkgCache *p = NULL;
	uint32_t flags = 0;
	int ret = MCPKG_MODRINTH_JSON_NO_ERROR;

	if (!versions_json || ver_idx < 0 || !out_cache || !provider)
		return MCPKG_MODRINTH_JSON_ERR_INVALID;

	tmp = (char *)malloc(versions_len + 1U);
	if (!tmp) return MCPKG_MODRINTH_JSON_ERR_NOMEM;
	memcpy(tmp, versions_json, versions_len);
	tmp[versions_len] = '\0';

	arr = cJSON_Parse(tmp);
	if (!cJSON_IsArray(arr)) {
		ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
		goto out;
	}
	ver = array_index(arr, ver_idx);
	if (!cJSON_IsObject(ver)) {
		ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
		goto out;
	}

	p = mcpkg_mp_pkg_meta_new();
	if (!p) {
		ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
		goto out;
	}

	/* from version */
	{
		const cJSON *project_id = cJSON_GetObjectItemCaseSensitive(ver, "project_id");
		const cJSON *version_id = cJSON_GetObjectItemCaseSensitive(ver, "id");
		const cJSON *version_no = cJSON_GetObjectItemCaseSensitive(ver,
		                          "version_number");
		const cJSON *loaders    = cJSON_GetObjectItemCaseSensitive(ver, "loaders");

		if (cJSON_IsString(project_id) && project_id->valuestring)
			p->id = dup_str(project_id->valuestring);
		if (!p->id) {
			ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
			goto out_err;
		}

		if (cJSON_IsString(version_no) && version_no->valuestring)
			p->version = dup_str(version_no->valuestring);
		if (!p->version) {
			ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
			goto out_err;
		}

		/* loaders (>=1) */
		if (cJSON_IsArray(loaders)) {
			const cJSON *e;
			p->loaders = mcpkg_stringlist_new(0, 0);
			if (!p->loaders) {
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out_err;
			}
			cJSON_ArrayForEach(e, loaders) {
				if (cJSON_IsString(e) && e->valuestring && e->valuestring[0]) {
					if (sl_push(p->loaders, e->valuestring) != 0) {
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}
				}
			}
		}
		if (!p->loaders) {
			ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
			goto out_err;
		}

		/* origin */
		{
			struct McPkgOrigin *o = mcpkg_mp_pkg_origin_new();
			const cJSON *files = cJSON_GetObjectItemCaseSensitive(ver, "files");
			const cJSON *primary = NULL;

			if (!o) {
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out_err;
			}

			o->provider   = dup_str(provider);
			o->project_id = dup_str(p->id);
			if (cJSON_IsString(version_id) && version_id->valuestring)
				o->version_id = dup_str(version_id->valuestring);

			if (cJSON_IsArray(files)) {
				const cJSON *f;
				cJSON_ArrayForEach(f, files) {
					const cJSON *is_primary = cJSON_GetObjectItemCaseSensitive(f, "primary");
					if (cJSON_IsBool(is_primary) && cJSON_IsTrue(is_primary)) {
						primary = f;
						break;
					}
				}
				if (!primary)
					primary = cJSON_GetArrayItem((cJSON *)files, 0);
				if (cJSON_IsObject(primary)) {
					const cJSON *url = cJSON_GetObjectItemCaseSensitive(primary, "url");
					if (cJSON_IsString(url) && url->valuestring)
						o->source_url = dup_str(url->valuestring);
				}
			}
			p->origin = o;
		}
	}

	/* enrich from hit */
	if (hit) {
		if (hit->slug && hit->slug[0]) p->slug = dup_str(hit->slug);
		if (hit->title && hit->title[0]) p->title = dup_str(hit->title);
		if (hit->description
		    && hit->description[0]) p->description = dup_str(hit->description);
		if (hit->license_id
		    && hit->license_id[0]) p->license_id = dup_str(hit->license_id);
		p->client = hit->client;
		p->server = hit->server;
		if (hit->categories) {
			size_t i, n = mcpkg_stringlist_size(hit->categories);
			p->sections = mcpkg_stringlist_new(0, 0);
			if (!p->sections) {
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out_err;
			}
			for (i = 0; i < n; i++) {
				const char *s = mcpkg_stringlist_at(hit->categories, i);
				if (s && s[0]) {
					if (sl_push(p->sections, s) != 0) {
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}
				}
			}
		}
	} else {
		const cJSON *proj_slug = cJSON_GetObjectItemCaseSensitive(ver, "project_slug");
		if (cJSON_IsString(proj_slug) && proj_slug->valuestring)
			p->slug = dup_str(proj_slug->valuestring);
	}

	/* files -> McPkgFile */
	{
		const cJSON *files = cJSON_GetObjectItemCaseSensitive(ver, "files");
		const cJSON *usef = NULL;

		if (!cJSON_IsArray(files)) {
			ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
			goto out_err;
		}

		{
			const cJSON *f;
			cJSON_ArrayForEach(f, files) {
				const cJSON *is_primary = cJSON_GetObjectItemCaseSensitive(f, "primary");
				if (cJSON_IsBool(is_primary) && cJSON_IsTrue(is_primary)) {
					usef = f;
					break;
				}
			}
			if (!usef) usef = cJSON_GetArrayItem((cJSON *)files, 0);
		}

		if (!cJSON_IsObject(usef)) {
			ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
			goto out_err;
		}

		{
			const cJSON *url  = cJSON_GetObjectItemCaseSensitive(usef, "url");
			const cJSON *name = cJSON_GetObjectItemCaseSensitive(usef, "filename");
			const cJSON *size = cJSON_GetObjectItemCaseSensitive(usef, "size");
			const cJSON *hash = cJSON_GetObjectItemCaseSensitive(usef, "hashes");

			struct McPkgFile *pf = mcpkg_mp_pkg_file_new();
			if (!pf) {
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out_err;
			}

			if (cJSON_IsString(url)
			    && url->valuestring)      pf->url       = dup_str(url->valuestring);
			if (cJSON_IsString(name)
			    && name->valuestring)    pf->file_name = dup_str(name->valuestring);
			if (cJSON_IsNumber(size) && size->valuedouble >= 0.0)
				pf->size = (uint64_t)size->valuedouble;

			if (!pf->url || !pf->file_name) {
				mcpkg_mp_pkg_file_free(pf);
				ret = MCPKG_MODRINTH_JSON_ERR_PARSE;
				goto out_err;
			}

			pf->digests = mcpkg_list_new(sizeof(struct McPkgDigest *), NULL, 0, 0);
			if (!pf->digests) {
				mcpkg_mp_pkg_file_free(pf);
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out_err;
			}

			if (cJSON_IsObject(hash)) {
				const cJSON *sha512 = cJSON_GetObjectItemCaseSensitive(hash, "sha512");
				const cJSON *sha1   = cJSON_GetObjectItemCaseSensitive(hash, "sha1");

				if (cJSON_IsString(sha512) && sha512->valuestring && sha512->valuestring[0]) {
					struct McPkgDigest *dg = mcpkg_mp_pkg_digest_new();
					if (!dg) {
						mcpkg_mp_pkg_file_free(pf);
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}
					dg->algo = 3u; /* SHA512 */
					dg->hex  = dup_str(sha512->valuestring);
					if (!dg->hex || mcpkg_list_push(pf->digests, &dg) != MCPKG_CONTAINER_OK) {
						mcpkg_mp_pkg_digest_free(dg);
						mcpkg_mp_pkg_file_free(pf);
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}
					flags |= MCPKG_MODRINTH_F_HAS_SHA512;
				}
				if (cJSON_IsString(sha1) && sha1->valuestring && sha1->valuestring[0]) {
					struct McPkgDigest *dg = mcpkg_mp_pkg_digest_new();
					if (!dg) {
						mcpkg_mp_pkg_file_free(pf);
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}
					dg->algo = 1u; /* SHA1 */
					dg->hex  = dup_str(sha1->valuestring);
					if (!dg->hex || mcpkg_list_push(pf->digests, &dg) != MCPKG_CONTAINER_OK) {
						mcpkg_mp_pkg_digest_free(dg);
						mcpkg_mp_pkg_file_free(pf);
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}
					flags |= MCPKG_MODRINTH_F_HAS_SHA1;
				}
			}

			p->file = pf;
		}
	}

	/* dependencies */
	{
		const cJSON *deps = cJSON_GetObjectItemCaseSensitive(ver, "dependencies");
		if (cJSON_IsArray(deps)) {
			const cJSON *d;
			p->depends = mcpkg_list_new(sizeof(struct McPkgDepends *), NULL, 0, 0);
			if (!p->depends) {
				ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
				goto out_err;
			}

			cJSON_ArrayForEach(d, deps) {
				const cJSON *proj = cJSON_GetObjectItemCaseSensitive(d, "project_id");
				const cJSON *verid = cJSON_GetObjectItemCaseSensitive(d, "version_id");
				const cJSON *kind = cJSON_GetObjectItemCaseSensitive(d, "dependency_type");

				if (cJSON_IsString(proj) && proj->valuestring && proj->valuestring[0]) {
					struct McPkgDepends *dp = mcpkg_mp_pkg_depends_new();
					if (!dp) {
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}

					dp->id = dup_str(proj->valuestring);
					if (!dp->id) {
						mcpkg_mp_pkg_depends_free(dp);
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}

					if (cJSON_IsString(verid) && verid->valuestring && verid->valuestring[0]) {
						size_t need = strlen(verid->valuestring) + 4;
						if (need < 256) {
							char vr[256];
							snprintf(vr, sizeof(vr), "id:%s", verid->valuestring);
							dp->version_range = dup_str(vr);
						} else {
							dp->version_range = dup_str("*");
						}
					} else {
						dp->version_range = dup_str("*");
					}

					dp->kind = dep_kind_from_str(cJSON_IsString(kind) ? kind->valuestring :
					                             "required");
					dp->side = -1;

					if (dp->kind == 2u) flags |= MCPKG_MODRINTH_F_HAS_INCOMPAT_DEPS;
					if (dp->kind == 3u) flags |= MCPKG_MODRINTH_F_HAS_EMBEDDED_DEPS;

					if (!dp->version_range ||
					    mcpkg_list_push(p->depends, &dp) != MCPKG_CONTAINER_OK) {
						mcpkg_mp_pkg_depends_free(dp);
						ret = MCPKG_MODRINTH_JSON_ERR_NOMEM;
						goto out_err;
					}
				}
			}
		}
	}

	*out_cache = p;
	p = NULL;
	if (out_flags) *out_flags = flags;
	ret = MCPKG_MODRINTH_JSON_NO_ERROR;
	goto out;

out_err:
	mcpkg_mp_pkg_meta_free(p);
out:
	if (arr) cJSON_Delete(arr);
	if (tmp) free(tmp);
	return ret;
}
