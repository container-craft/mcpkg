/* SPDX-License-Identifier: MIT */
#include "net/modrinth/mcpkg_net_modrinth_client.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <cjson/cJSON.h>
#include "net/mcpkg_net_util.h"
#include "net/mcpkg_net_url.h"

#include "container/mcpkg_str_list.h"
#include "container/mcpkg_list.h"

/* mpgen structs */
#include "mp/mcpkg_mp_pkg_meta.h"
#include "mp/mcpkg_mp_pkg_file.h"
#include "mp/mcpkg_mp_pkg_digest.h"
#include "mp/mcpkg_mp_pkg_depends.h"
#include "mp/mcpkg_mp_pkg_origin.h"

struct McPkgModrinthClient {
	McPkgNetClient  *net;
};

/* ---- utils ---- */

static char *dup_cstr(const char *s)
{
	size_t n;
	char *d;

	if (!s) return NULL;
	n = strlen(s) + 1U;
	d = (char *)malloc(n);
	if (!d) return NULL;
	memcpy(d, s, n);
	return d;
}

/* RFC 3986 encode for query value; keep ALPHA / DIGIT / -._~ */
static char *urlenc_component(const char *s)
{
	const char *hex = "0123456789ABCDEF";
	size_t i, n, cap;
	char *o;

	if (!s) return dup_cstr("");
	n = strlen(s);
	cap = n * 3 + 1;
	o = (char *)malloc(cap);
	if (!o) return NULL;

	size_t k = 0;
	for (i = 0; i < n; i++) {
		unsigned char c = (unsigned char)s[i];
		int unres = (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~');
		if (unres) {
			o[k++] = (char)c;
		} else {
			o[k++] = '%';
			o[k++] = hex[(c >> 4) & 0xF];
			o[k++] = hex[c & 0xF];
		}
	}
	o[k] = '\0';
	return o;
}

static char *fmt_facets(const char *loader, const char *mc_version)
{
	/* [[ "categories:<loader>" ], [ "versions:<mc_version>" ]] */
	const char *l = loader ? loader : "";
	const char *v = mc_version ? mc_version : "";
	size_t need =
	        4 + 2 + 12 + strlen(l) + 4 + /* ["categories:<l>"], */
	        4 + 2 + 10 + strlen(v) + 3 + /* ["versions:<v>"]  */
	        3;                            /* outer [] + NUL */
	char *raw = (char *)malloc(need);
	if (!raw) return NULL;
	/* keep compact to reduce encoding size */
	snprintf(raw, need, "[[\"categories:%s\"],[\"versions:%s\"]]", l, v);
	return raw;
}

static char *fmt_json_array_1(const char *elt)
{
	const char *e = elt ? elt : "";
	size_t need = 2 + 2 + strlen(e) + 2; /* ["elt"] + NUL */
	char *s = (char *)malloc(need);
	if (!s) return NULL;
	snprintf(s, need, "[\"%s\"]", e);
	return s;
}

/* map client/server side string to tri-state: UNKNOWN=-1, NO=0, YES=1 */
static int str_to_tristate(const char *s)
{
	if (!s || !*s) return -1;
	if (!strcmp(s, "required") || !strcmp(s, "optional")) return 1;
	if (!strcmp(s, "unsupported")) return 0;
	return -1;
}

/* categories -> sections (strip loader tokens) */
static int fill_sections_from_categories(cJSON *cats, const char *loader,
                struct McPkgStringList **out_sl)
{
	static const char *known_loaders[] = { "fabric", "forge", "neoforge", "quilt", "babric", "minecraft", NULL };
	struct McPkgStringList *sl = NULL;
	size_t i, n;

	if (!out_sl) return MCPKG_MODR_ERR_INVALID;
	*out_sl = NULL;

	if (!cJSON_IsArray(cats))
		return MCPKG_MODR_NO_ERROR;

	sl = mcpkg_stringlist_new(0, 0);
	if (!sl) return MCPKG_MODR_ERR_NOMEM;

	n = (size_t)cJSON_GetArraySize(cats);
	for (i = 0; i < n; i++) {
		cJSON *e = cJSON_GetArrayItem(cats, (int)i);
		const char *v = cJSON_IsString(e) ? e->valuestring : NULL;
		int is_loader = 0, j;

		if (!v || !*v) continue;
		if (loader && !strcmp(v, loader)) continue;

		for (j = 0; known_loaders[j]; j++) {
			if (!strcmp(v, known_loaders[j])) {
				is_loader = 1;
				break;
			}
		}
		if (is_loader) continue;

		if (mcpkg_stringlist_push(sl, v) != MCPKG_CONTAINER_OK) {
			mcpkg_stringlist_free(sl);
			return MCPKG_MODR_ERR_NOMEM;
		}
	}

	*out_sl = sl;
	return MCPKG_MODR_NO_ERROR;
}

static struct McPkgDigest *mk_digest_u32(uint32_t algo, const char *hex)
{
	struct McPkgDigest *d = mcpkg_mp_pkg_digest_new();
	if (!d) return NULL;
	d->algo = algo;
	d->hex = hex ? dup_cstr(hex) : dup_cstr("");
	if (!d->hex) {
		mcpkg_mp_pkg_digest_free(d);
		return NULL;
	}
	return d;
}

/* choose "primary" file; if none marked, pick index 0 */
static cJSON *pick_primary_file(cJSON *files)
{
	int i, n;
	if (!cJSON_IsArray(files)) return NULL;
	n = cJSON_GetArraySize(files);
	for (i = 0; i < n; i++) {
		cJSON *f = cJSON_GetArrayItem(files, i);
		cJSON *prim = cJSON_GetObjectItemCaseSensitive(f, "primary");
		if (cJSON_IsBool(prim) && cJSON_IsTrue(prim))
			return f;
	}
	return n > 0 ? cJSON_GetArrayItem(files, 0) : NULL;
}

static int build_pkg_from_hit_and_version(const char *loader,
                const char *mc_version,
                cJSON *hit, cJSON *ver,
                struct McPkgCache **out_pkg)
{
	struct McPkgCache *p = NULL;
	struct McPkgStringList *loaders = NULL, *sections = NULL;
	cJSON *files = NULL, *file = NULL, *hashes = NULL;
	struct McPkgFile *pf = NULL;
	struct McPkgOrigin *po = NULL;
	cJSON *deps = NULL;
	int i, n;

	if (!out_pkg) return MCPKG_MODR_ERR_INVALID;
	*out_pkg = NULL;
	if (!cJSON_IsObject(hit) || !cJSON_IsObject(ver))
		return MCPKG_MODR_ERR_PARSE;

	p = mcpkg_mp_pkg_meta_new();
	if (!p) return MCPKG_MODR_ERR_NOMEM;

	/* id, slug, title, description, license (from hit) */
	{
		cJSON *v;

		v = cJSON_GetObjectItemCaseSensitive(hit, "project_id");
		p->id = dup_cstr(cJSON_IsString(v) ? v->valuestring : NULL);

		v = cJSON_GetObjectItemCaseSensitive(hit, "slug");
		p->slug = dup_cstr(cJSON_IsString(v) ? v->valuestring : NULL);

		v = cJSON_GetObjectItemCaseSensitive(hit, "title");
		p->title = dup_cstr(cJSON_IsString(v) ? v->valuestring : NULL);

		v = cJSON_GetObjectItemCaseSensitive(hit, "description");
		p->description = dup_cstr(cJSON_IsString(v) ? v->valuestring : NULL);

		v = cJSON_GetObjectItemCaseSensitive(hit, "license");
		p->license_id = dup_cstr(cJSON_IsString(v) ? v->valuestring : NULL);

		v = cJSON_GetObjectItemCaseSensitive(hit, "client_side");
		p->client = str_to_tristate(cJSON_IsString(v) ? v->valuestring : NULL);

		v = cJSON_GetObjectItemCaseSensitive(hit, "server_side");
		p->server = str_to_tristate(cJSON_IsString(v) ? v->valuestring : NULL);

		/* sections from categories */
		v = cJSON_GetObjectItemCaseSensitive(hit, "categories");
		if (fill_sections_from_categories(v, loader,
		                                  &sections) != MCPKG_MODR_NO_ERROR) {
			mcpkg_mp_pkg_meta_free(p);
			return MCPKG_MODR_ERR_NOMEM;
		}
		p->sections = sections;
	}

	/* version_number (as meta.version) */
	{
		cJSON *vv = cJSON_GetObjectItemCaseSensitive(ver, "version_number");
		p->version = dup_cstr(cJSON_IsString(vv) ? vv->valuestring : NULL);
	}

	/* loaders list: just the selected loader (>=1 required) */
	loaders = mcpkg_stringlist_new(0, 0);
	if (!loaders) {
		mcpkg_mp_pkg_meta_free(p);
		return MCPKG_MODR_ERR_NOMEM;
	}
	if (mcpkg_stringlist_push(loaders,
	                          loader ? loader : "") != MCPKG_CONTAINER_OK) {
		mcpkg_stringlist_free(loaders);
		mcpkg_mp_pkg_meta_free(p);
		return MCPKG_MODR_ERR_NOMEM;
	}
	p->loaders = loaders;

	/* file */
	files = cJSON_GetObjectItemCaseSensitive(ver, "files");
	file = pick_primary_file(files);
	if (!cJSON_IsObject(file)) {
		mcpkg_mp_pkg_meta_free(p);
		return MCPKG_MODR_ERR_PARSE;
	}

	pf = mcpkg_mp_pkg_file_new();
	if (!pf) {
		mcpkg_mp_pkg_meta_free(p);
		return MCPKG_MODR_ERR_NOMEM;
	}

	{
		cJSON *v;
		v = cJSON_GetObjectItemCaseSensitive(file, "url");
		pf->url = dup_cstr(cJSON_IsString(v) ? v->valuestring : NULL);
		v = cJSON_GetObjectItemCaseSensitive(file, "filename");
		pf->file_name = dup_cstr(cJSON_IsString(v) ? v->valuestring : NULL);
		v = cJSON_GetObjectItemCaseSensitive(file, "size");
		if (cJSON_IsNumber(v)) pf->size = (uint64_t)v->valuedouble;

		/* digests list<struct McPkgDigest *> */
		hashes = cJSON_GetObjectItemCaseSensitive(file, "hashes");
		if (cJSON_IsObject(hashes)) {
			struct McPkgList *dl = mcpkg_list_new(sizeof(struct McPkgDigest *), NULL, 0, 0);
			if (!dl) {
				mcpkg_mp_pkg_file_free(pf);
				mcpkg_mp_pkg_meta_free(p);
				return MCPKG_MODR_ERR_NOMEM;
			}

			/* sha512 */
			v = cJSON_GetObjectItemCaseSensitive(hashes, "sha512");
			if (cJSON_IsString(v) && v->valuestring && *v->valuestring) {
				struct McPkgDigest *d = mk_digest_u32(3 /* SHA512 */, v->valuestring);
				if (!d || mcpkg_list_push(dl, &d) != MCPKG_CONTAINER_OK) {
					if (d) mcpkg_mp_pkg_digest_free(d);
					mcpkg_list_free(dl);
					mcpkg_mp_pkg_file_free(pf);
					mcpkg_mp_pkg_meta_free(p);
					return MCPKG_MODR_ERR_NOMEM;
				}
			}
			/* sha1 (compat) */
			v = cJSON_GetObjectItemCaseSensitive(hashes, "sha1");
			if (cJSON_IsString(v) && v->valuestring && *v->valuestring) {
				struct McPkgDigest *d = mk_digest_u32(1 /* SHA1 */, v->valuestring);
				if (!d || mcpkg_list_push(dl, &d) != MCPKG_CONTAINER_OK) {
					if (d) mcpkg_mp_pkg_digest_free(d);
					mcpkg_list_free(dl);
					mcpkg_mp_pkg_file_free(pf);
					mcpkg_mp_pkg_meta_free(p);
					return MCPKG_MODR_ERR_NOMEM;
				}
			}
			pf->digests = dl;
		}
	}
	p->file = pf;

	/* depends */
	deps = cJSON_GetObjectItemCaseSensitive(ver, "dependencies");
	if (cJSON_IsArray(deps)) {
		struct McPkgList *dl = mcpkg_list_new(sizeof(struct McPkgDepends *), NULL, 0,
		                                      0);
		if (!dl) {
			mcpkg_mp_pkg_meta_free(p);
			return MCPKG_MODR_ERR_NOMEM;
		}

		n = cJSON_GetArraySize(deps);
		for (i = 0; i < n; i++) {
			cJSON *d = cJSON_GetArrayItem(deps, i);
			cJSON *pid = cJSON_GetObjectItemCaseSensitive(d, "project_id");
			cJSON *vid = cJSON_GetObjectItemCaseSensitive(d, "version_id");
			cJSON *dt  = cJSON_GetObjectItemCaseSensitive(d, "dependency_type");
			const char *id = cJSON_IsString(pid) ? pid->valuestring : NULL;
			const char *vids = cJSON_IsString(vid) ? vid->valuestring : NULL;
			const char *kind = cJSON_IsString(dt)  ? dt->valuestring  : NULL;
			uint32_t kcode = 0;

			if (!id || !*id) id = vids; /* fall back to version id */
			if (!id || !*id) continue;  /* skip if nothing */

			if (kind) {
				if (!strcmp(kind, "required")) kcode = 0;
				else if (!strcmp(kind, "optional")) kcode = 1;
				else if (!strcmp(kind, "incompatible")) kcode = 2;
				else if (!strcmp(kind, "embedded")) kcode = 3;
			}

			{
				struct McPkgDepends *pd = mcpkg_mp_pkg_depends_new();
				if (!pd) {
					mcpkg_list_free(dl);
					mcpkg_mp_pkg_meta_free(p);
					return MCPKG_MODR_ERR_NOMEM;
				}
				pd->id = dup_cstr(id);
				pd->version_range = dup_cstr("*"); /* Modrinth doesn't give ranges here */
				pd->kind = kcode;
				pd->side = -1;

				if (!pd->id || !pd->version_range ||
				    mcpkg_list_push(dl, &pd) != MCPKG_CONTAINER_OK) {
					mcpkg_mp_pkg_depends_free(pd);
					mcpkg_list_free(dl);
					mcpkg_mp_pkg_meta_free(p);
					return MCPKG_MODR_ERR_NOMEM;
				}
			}
		}
		p->depends = dl;
	}

	/* origin */
	{
		cJSON *vid = cJSON_GetObjectItemCaseSensitive(ver, "id");
		const char *version_id = cJSON_IsString(vid) ? vid->valuestring : NULL;

		po = mcpkg_mp_pkg_origin_new();
		if (!po) {
			mcpkg_mp_pkg_meta_free(p);
			return MCPKG_MODR_ERR_NOMEM;
		}
		po->provider   = dup_cstr("modrinth");
		po->project_id = dup_cstr(p->id ? p->id : "");
		po->version_id = dup_cstr(version_id ? version_id : "");
		po->source_url = p->file && p->file->url ? dup_cstr(p->file->url) : NULL;

		if (!po->provider || !po->project_id) {
			mcpkg_mp_pkg_origin_free(po);
			mcpkg_mp_pkg_meta_free(p);
			return MCPKG_MODR_ERR_NOMEM;
		}
		p->origin = po;
	}

	*out_pkg = p;
	return MCPKG_MODR_NO_ERROR;
}

/* ---- lifecycle ---- */

MCPKG_API McPkgModrinthClient *
mcpkg_net_modrinth_client_new(const McPkgModrinthClientCfg *cfg)
{
	McPkgModrinthClient *mc = NULL;
	McPkgNetClientCfg nc = {0};

	if (!cfg || !cfg->base_url || !cfg->base_url[0])
		return NULL;

	mc = (McPkgModrinthClient *)calloc(1, sizeof(*mc));
	if (!mc) return NULL;

	nc.base_url = cfg->base_url;
	nc.user_agent = cfg->user_agent;
	nc.default_headers = cfg->default_headers;
	nc.connect_timeout_ms = cfg->connect_timeout_ms;
	nc.operation_timeout_ms = cfg->operation_timeout_ms;

	mc->net = mcpkg_net_client_new(&nc);
	if (!mc->net) {
		free(mc);
		return NULL;
	}

	/* Ensure JSON Accept by default if not already set (best-effort) */
	(void)mcpkg_net_client_set_header(mc->net, "Accept: application/json");

	return mc;
}

MCPKG_API void
mcpkg_net_modrinth_client_free(McPkgModrinthClient *c)
{
	if (!c) return;
	if (c->net) mcpkg_net_client_free(c->net);
	free(c);
}

MCPKG_API McPkgNetRateLimit
mcpkg_net_modrinth_get_ratelimit(McPkgModrinthClient *c)
{
	return mcpkg_net_get_ratelimit(c ? c->net : NULL);
}

/* ---- RAW FETCHERS ---- */

MCPKG_API int
mcpkg_net_modrinth_search_raw(McPkgModrinthClient *c,
                              const char *loader,
                              const char *mc_version,
                              int limit, int offset,
                              struct McPkgNetBuf *out_body,
                              long *out_http)
{
	char limit_s[32], offset_s[32];
	char *facets_raw = NULL, *facets_enc = NULL;
	const char *path = "/v2/search";
	const char *qv[8] = {0};
	int qi = 0;
	int ret;

	if (!c || !out_body) return MCPKG_MODR_ERR_INVALID;

	if (limit <= 0) limit = 100;
	if (offset < 0) offset = 0;
	snprintf(limit_s, sizeof(limit_s), "%d", limit);
	snprintf(offset_s, sizeof(offset_s), "%d", offset);

	facets_raw = fmt_facets(loader, mc_version);
	if (!facets_raw) return MCPKG_MODR_ERR_NOMEM;
	facets_enc = urlenc_component(facets_raw);
	free(facets_raw);
	if (!facets_enc) return MCPKG_MODR_ERR_NOMEM;

	qv[qi++] = "facets";
	qv[qi++] = facets_enc;
	qv[qi++] = "limit";
	qv[qi++] = limit_s;
	qv[qi++] = "offset";
	qv[qi++] = offset_s;
	qv[qi++] = NULL;

	ret = mcpkg_net_request(c->net, "GET", path, qv, NULL, 0, out_body, out_http);
	free(facets_enc);

	if (ret != MCPKG_NET_NO_ERROR)
		return MCPKG_MODR_ERR_HTTP;

	return MCPKG_MODR_NO_ERROR;
}

MCPKG_API int
mcpkg_net_modrinth_versions_raw(McPkgModrinthClient *c,
                                const char *id_or_slug,
                                const char *loader,
                                const char *mc_version,
                                struct McPkgNetBuf *out_body,
                                long *out_http)
{
	char *gv_raw = NULL, *gv_enc = NULL;
	char *ld_raw = NULL, *ld_enc = NULL;
	char path[256];
	const char *qv[6] = {0};
	int qi = 0;
	int ret;

	if (!c || !id_or_slug || !*id_or_slug || !out_body)
		return MCPKG_MODR_ERR_INVALID;

	snprintf(path, sizeof(path), "/v2/project/%s/version", id_or_slug);

	gv_raw = fmt_json_array_1(mc_version);
	ld_raw = fmt_json_array_1(loader);
	if (!gv_raw || !ld_raw) {
		free(gv_raw);
		free(ld_raw);
		return MCPKG_MODR_ERR_NOMEM;
	}

	gv_enc = urlenc_component(gv_raw);
	ld_enc = urlenc_component(ld_raw);
	free(gv_raw);
	free(ld_raw);
	if (!gv_enc || !ld_enc) {
		free(gv_enc);
		free(ld_enc);
		return MCPKG_MODR_ERR_NOMEM;
	}

	qv[qi++] = "game_versions";
	qv[qi++] = gv_enc;
	qv[qi++] = "loaders";
	qv[qi++] = ld_enc;
	qv[qi++] = NULL;

	ret = mcpkg_net_request(c->net, "GET", path, qv, NULL, 0, out_body, out_http);

	free(gv_enc);
	free(ld_enc);

	if (ret != MCPKG_NET_NO_ERROR)
		return MCPKG_MODR_ERR_HTTP;

	return MCPKG_MODR_NO_ERROR;
}

/* ---- HIGH-LEVEL PAGE BUILDER ---- */

MCPKG_API int
mcpkg_net_modrinth_fetch_page_build(McPkgModrinthClient *c,
                                    const char *loader,
                                    const char *mc_version,
                                    int limit, int offset,
                                    struct McPkgList **out_pkgs)
{
	struct McPkgNetBuf sb = {0};
	long http = 0;
	cJSON *root = NULL, *hits = NULL;
	int h, hcount;
	struct McPkgList *lst = NULL;
	int ret;

	if (!c || !out_pkgs || !loader || !*loader || !mc_version || !*mc_version)
		return MCPKG_MODR_ERR_INVALID;

	ret = mcpkg_net_modrinth_search_raw(c, loader, mc_version, limit, offset, &sb,
	                                    &http);
	if (ret != MCPKG_MODR_NO_ERROR) return ret;
	if (http != 200) {
		mcpkg_net_buf_free(&sb);
		return MCPKG_MODR_ERR_HTTP;
	}

	root = cJSON_ParseWithLength((const char *)sb.data, (size_t)sb.len);
	if (!root) {
		mcpkg_net_buf_free(&sb);
		return MCPKG_MODR_ERR_JSON;
	}

	hits = cJSON_GetObjectItemCaseSensitive(root, "hits");
	if (!cJSON_IsArray(hits)) {
		cJSON_Delete(root);
		mcpkg_net_buf_free(&sb);
		return MCPKG_MODR_ERR_PARSE;
	}

	lst = mcpkg_list_new(sizeof(struct McPkgCache *), NULL, 0, 0);
	if (!lst) {
		cJSON_Delete(root);
		mcpkg_net_buf_free(&sb);
		return MCPKG_MODR_ERR_NOMEM;
	}

	hcount = cJSON_GetArraySize(hits);
	for (h = 0; h < hcount; h++) {
		cJSON *hit = cJSON_GetArrayItem(hits, h);
		cJSON *pid = cJSON_GetObjectItemCaseSensitive(hit, "project_id");
		cJSON *slug = cJSON_GetObjectItemCaseSensitive(hit, "slug");
		const char *id_or_slug =
		        (cJSON_IsString(slug) && slug->valuestring && *slug->valuestring)
		        ? slug->valuestring
		        : (cJSON_IsString(pid) ? pid->valuestring : NULL);

		struct McPkgNetBuf vb = {0};
		long vhttp = 0;
		cJSON *vers = NULL, *ver0 = NULL;
		struct McPkgCache *pkg = NULL;

		if (!id_or_slug || !*id_or_slug) continue;

		ret = mcpkg_net_modrinth_versions_raw(c, id_or_slug, loader, mc_version, &vb,
		                                      &vhttp);
		if (ret != MCPKG_MODR_NO_ERROR) { /* skip this hit on transient error */
			mcpkg_net_buf_free(&vb);
			continue;
		}
		if (vhttp != 200) {
			mcpkg_net_buf_free(&vb);
			continue;
		}

		vers = cJSON_ParseWithLength((const char *)vb.data, (size_t)vb.len);
		if (!vers) {
			mcpkg_net_buf_free(&vb);
			continue;
		}
		/* versions endpoint returns an array */
		if (!cJSON_IsArray(vers) || cJSON_GetArraySize(vers) <= 0) {
			cJSON_Delete(vers);
			mcpkg_net_buf_free(&vb);
			continue;
		}
		ver0 = cJSON_GetArrayItem(vers, 0);

		ret = build_pkg_from_hit_and_version(loader, mc_version, hit, ver0, &pkg);
		cJSON_Delete(vers);
		mcpkg_net_buf_free(&vb);
		if (ret != MCPKG_MODR_NO_ERROR || !pkg) continue;

		if (mcpkg_list_push(lst, &pkg) != MCPKG_CONTAINER_OK) {
			mcpkg_mp_pkg_meta_free(pkg);
			/* keep going; if too many failures, caller still gets partials */
		}
	}

	cJSON_Delete(root);
	mcpkg_net_buf_free(&sb);

	*out_pkgs = lst;
	return MCPKG_MODR_NO_ERROR;
}
