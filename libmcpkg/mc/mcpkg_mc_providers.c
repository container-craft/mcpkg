// Abstract provider registry and helpers.

#include <stdlib.h>
#include <string.h>
#include "mcpkg_mc_providers.h"
#include "mp/mcpkg_mp_util.h"
#include "mcpkg_mc_util.h"
// int-keyed fields (0/1 reserved for common TAG/VER)
#define MC_PROV_K_PROVIDER    2
#define MC_PROV_K_NAME        3
#define MC_PROV_K_BASE_URL    4
#define MC_PROV_K_ONLINE      5
#define MC_PROV_K_FLAGS       6

static const struct McPkgMcProvider g_provider_table[] = {
	{
		MCPKG_MC_PROVIDER_MODRINTH,
		"modrinth",
		"https://api.modrinth.com",
		1,
		MCPKG_MC_PROVIDER_F_ONLINE_REQUIRED | MCPKG_MC_PROVIDER_F_HAS_API |
		MCPKG_MC_PROVIDER_F_PROVIDES_INDEX | MCPKG_MC_PROVIDER_F_SUPPORTS_CLIENT |
		MCPKG_MC_PROVIDER_F_SUPPORTS_SERVER, NULL, NULL, 0
	},
	{
		MCPKG_MC_PROVIDER_CURSEFORGE,
		"curseforge",
		"https://api.curseforge.com",
		1,
		MCPKG_MC_PROVIDER_F_ONLINE_REQUIRED | MCPKG_MC_PROVIDER_F_HAS_API |
		MCPKG_MC_PROVIDER_F_PROVIDES_INDEX | MCPKG_MC_PROVIDER_F_SUPPORTS_CLIENT |
		MCPKG_MC_PROVIDER_F_SUPPORTS_SERVER, NULL, NULL, 0
	},
	{
		MCPKG_MC_PROVIDER_HANGAR,
		"hangar",
		"https://hangar.papermc.io",
		1,
		MCPKG_MC_PROVIDER_F_ONLINE_REQUIRED | MCPKG_MC_PROVIDER_F_HAS_API | MCPKG_MC_PROVIDER_F_SUPPORTS_SERVER,
		NULL,
		NULL, 0
	},
	{
		MCPKG_MC_PROVIDER_LOCAL,
		"local",
		NULL,
		1,
		MCPKG_MC_PROVIDER_F_PROVIDES_INDEX | MCPKG_MC_PROVIDER_F_SUPPORTS_CLIENT | MCPKG_MC_PROVIDER_F_SUPPORTS_SERVER,
		NULL,
		NULL,
		0
	},
};

static size_t mcpkg_mc_provider_table_count(void)
{
	return sizeof(g_provider_table) / sizeof(g_provider_table[0]);
}

static const struct McPkgMcProvider *
mcpkg_mc_provider_find_by_id(MCPKG_MC_PROVIDERS id)
{
	size_t i, n = mcpkg_mc_provider_table_count();

	for (i = 0; i < n; i++) {
		if (g_provider_table[i].provider == id)
			return &g_provider_table[i];
	}
	return NULL;
}

static const struct McPkgMcProvider *
mcpkg_mc_provider_find_by_name(const char *s)
{
	size_t i, n;

	if (!s)
		return NULL;

	n = mcpkg_mc_provider_table_count();
	for (i = 0; i < n; i++) {
		if (mcpkg_mc_ascii_ieq(g_provider_table[i].name, s))
			return &g_provider_table[i];
	}
	return NULL;
}

// Heap constructor: outp gets an owned instance initialized from the template.
int mcpkg_mc_provider_new(struct McPkgMcProvider **outp, MCPKG_MC_PROVIDERS id)
{
	int ret = MCPKG_MC_NO_ERROR;
	const struct McPkgMcProvider *tmpl;
	struct McPkgMcProvider *p;

	if (!outp) {
		ret = MCPKG_MC_ERR_INVALID_ARG;
		goto out;
	}

	tmpl = mcpkg_mc_provider_find_by_id(id);
	if (!tmpl) {
		ret = MCPKG_MC_ERR_NOT_FOUND;
		goto out;
	}

	p = (struct McPkgMcProvider *)malloc(sizeof(*p));
	if (!p) {
		ret = MCPKG_MC_ERR_NO_MEMORY;
		goto out;
	}

	*p = *tmpl;
	p->owns_base_url = 0;
	p->priv = NULL;
	*outp = p;

out:
	return ret;
}

void mcpkg_mc_provider_free(struct McPkgMcProvider *p)
{
	if (!p)
		return;

	if (p->ops && p->ops->destroy)
		p->ops->destroy(p);

	if (p->owns_base_url && p->base_url) {
		free((void *)p->base_url);
		p->base_url = NULL;
		p->owns_base_url = 0;
	}

	free(p);
}

struct McPkgMcProvider mcpkg_mc_provider_make(MCPKG_MC_PROVIDERS id)
{
	const struct McPkgMcProvider *hit;

	hit = mcpkg_mc_provider_find_by_id(id);
	if (hit)
		return *hit;

	return (struct McPkgMcProvider) {
		.provider = MCPKG_MC_PROVIDER_UNKNOWN,
		.name = mcpkg_mc_string_unknown(),
		.base_url = NULL,
		.online = 0,
		.flags = 0,
		.ops = NULL,
		.priv = NULL,
		.owns_base_url = 0,
	};
}

struct McPkgMcProvider
mcpkg_mc_provider_from_string_canon(const char *s)
{
	const struct McPkgMcProvider *hit;

	hit = mcpkg_mc_provider_find_by_name(s);
	if (hit)
		return *hit;

	return mcpkg_mc_provider_make(MCPKG_MC_PROVIDER_UNKNOWN);
}

const char *mcpkg_mc_provider_to_string(MCPKG_MC_PROVIDERS id)
{
	const struct McPkgMcProvider *hit;

	hit = mcpkg_mc_provider_find_by_id(id);
	return hit ? hit->name : mcpkg_mc_string_unknown();
}

MCPKG_MC_PROVIDERS mcpkg_mc_provider_from_string(const char *s)
{
	const struct McPkgMcProvider *hit;

	hit = mcpkg_mc_provider_find_by_name(s);
	return hit ? hit->provider : MCPKG_MC_PROVIDER_UNKNOWN;
}

const struct McPkgMcProvider *
mcpkg_mc_provider_table(size_t *count)
{
	if (count)
		*count = mcpkg_mc_provider_table_count();
	return g_provider_table;
}

// >0 known, 0 unknown
int mcpkg_mc_provider_is_known(MCPKG_MC_PROVIDERS id)
{
	return mcpkg_mc_provider_find_by_id(id) != NULL;
}

// >0 requires network, 0 does not, <0 error
int mcpkg_mc_provider_requires_network(const struct McPkgMcProvider *p)
{
	if (!p)
		return MCPKG_MC_ERR_INVALID_ARG;
	return (p->flags & MCPKG_MC_PROVIDER_F_ONLINE_REQUIRED) != 0;
}

// >0 online, 0 offline, <0 error
int mcpkg_mc_provider_is_online(struct McPkgMcProvider *p)
{
	int ret = 0;

	if (!p) {
		ret = MCPKG_MC_ERR_INVALID_ARG;
		goto out;
	}

	if (p->ops && p->ops->is_online) {
		ret = p->ops->is_online(p);
		goto out;
	}

	ret = p->online ? 1 : 0;

out:
	return ret;
}

void mcpkg_mc_provider_set_base_url(struct McPkgMcProvider *p,
                                    const char *base_url)
{
	if (!p)
		return;

	if (p->owns_base_url && p->base_url) {
		free((void *)p->base_url);
		p->base_url = NULL;
		p->owns_base_url = 0;
	}

	p->base_url = base_url;
}

int mcpkg_mc_provider_set_base_url_dup(struct McPkgMcProvider *p,
                                       const char *base_url)
{
	int ret = MCPKG_MC_NO_ERROR;
	char *dup = NULL;

	if (!p)
		return MCPKG_MC_ERR_INVALID_ARG;

	if (p->owns_base_url && p->base_url) {
		free((void *)p->base_url);
		p->base_url = NULL;
		p->owns_base_url = 0;
	}

	if (base_url) {
		dup = strdup(base_url);
		if (!dup) {
			ret = MCPKG_MC_ERR_NO_MEMORY;
			goto out;
		}
		p->base_url = dup;
		p->owns_base_url = 1;
	}

out:
	return ret;
}

void mcpkg_mc_provider_set_online(struct McPkgMcProvider *p, int online)
{
	if (!p)
		return;
	p->online = online ? 1 : 0;
}

void mcpkg_mc_provider_set_ops(struct McPkgMcProvider *p,
                               const struct McPkgMcProviderOps *ops)
{
	if (!p)
		return;
	p->ops = ops;
}


// static int mcpkg_mc_err_from_mcpkg_pack_err(int e)
// {
//     switch (e) {
//     case MCPKG_MP_NO_ERROR:        return MCPKG_MC_NO_ERROR;
//     case MCPKG_MP_ERR_INVALID_ARG: return MCPKG_MC_ERR_INVALID_ARG;
//     case MCPKG_MP_ERR_PARSE:       return MCPKG_MC_ERR_PARSE;
//     case MCPKG_MP_ERR_NO_MEMORY:   return MCPKG_MC_ERR_NO_MEMORY;
//     case MCPKG_MP_ERR_IO:          return MCPKG_MC_ERR_IO;
//     default:                       return MCPKG_MC_ERR_PARSE;
//     }
// }

int mcpkg_mc_provider_pack(const struct McPkgMcProvider *p,
                           void **out_buf, size_t *out_len)
{
	int ret = MCPKG_MC_NO_ERROR;
	struct McPkgMpWriter w;
	int mpret;

	if (!p || !out_buf || !out_len)
		return MCPKG_MC_ERR_INVALID_ARG;

	mpret = mcpkg_mp_writer_init(&w);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out;
	}

	// 2 header keys (TAG, VER) + 5 fields
	mpret = mcpkg_mp_map_begin(&w, 7);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_write_header(&w, MCPKG_MC_PROVIDER_MP_TAG,
	                              MCPKG_MC_PROVIDER_MP_VERSION);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_kv_i32(&w, MC_PROV_K_PROVIDER, (int)p->provider);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	// Store canonical name for debugging/inspection (reader may ignore/replace)
	mpret = mcpkg_mp_kv_str(&w, MC_PROV_K_NAME, p->name);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	if (p->base_url)
		mpret = mcpkg_mp_kv_str(&w, MC_PROV_K_BASE_URL, p->base_url);
	else
		mpret = mcpkg_mp_kv_nil(&w, MC_PROV_K_BASE_URL);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_kv_i32(&w, MC_PROV_K_ONLINE, p->online ? 1 : 0);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_kv_u32(&w, MC_PROV_K_FLAGS, p->flags);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_writer_finish(&w, out_buf, out_len);
	if (mpret != MCPKG_MP_NO_ERROR)
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);

out_w:
	mcpkg_mp_writer_destroy(&w);
out:
	return ret;
}

int mcpkg_mc_provider_unpack(const void *buf, size_t len,
                             struct McPkgMcProvider *out)
{
	int ret = MCPKG_MC_NO_ERROR;
	struct McPkgMpReader r;
	int mpret, found, ver = 0;
	int64_t i64 = 0;
	uint32_t u32 = 0;
	const char *sp = NULL;
	size_t slen = 0;

	MCPKG_MC_PROVIDERS provider = MCPKG_MC_PROVIDER_UNKNOWN;
	int online = 0;
	uint32_t flags = 0;
	const char *base_url_ptr = NULL;
	size_t base_url_len = 0;

	if (!buf || !len || !out)
		return MCPKG_MC_ERR_INVALID_ARG;

	mpret = mcpkg_mp_reader_init(&r, buf, len);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out;
	}

	mpret = mcpkg_mp_expect_tag(&r, MCPKG_MC_PROVIDER_MP_TAG, &ver);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}
	// version's is currently unused; keep for future evolution.

	// provider (required)
	mpret = mcpkg_mp_get_i64(&r, MC_PROV_K_PROVIDER, &i64, &found);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}
	if (!found) {
		ret = MCPKG_MC_ERR_PARSE;
		goto out_r;
	}
	provider = (MCPKG_MC_PROVIDERS)i64;

	// name (optional) — we keep canonical name from template; ignore blob name
	(void)mcpkg_mp_get_str_borrow(&r, MC_PROV_K_NAME, &sp, &slen, &found);

	// base_url (optional)
	mpret = mcpkg_mp_get_str_borrow(&r, MC_PROV_K_BASE_URL,
	                                &base_url_ptr, &base_url_len, &found);
	if (mpret != MCPKG_MC_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}

	// online (optional -> default 0)
	mpret = mcpkg_mp_get_i64(&r, MC_PROV_K_ONLINE, &i64, &found);
	if (mpret != MCPKG_MC_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}
	if (found) online = (i64 != 0);

	// flags (optional -> default 0)
	mpret = mcpkg_mp_get_u32(&r, MC_PROV_K_FLAGS, &u32, &found);
	if (mpret != MCPKG_MC_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}
	if (found)
		flags = u32;

	// Start from template defaults
	*out = mcpkg_mc_provider_make(provider);

	// base_url: free prior owned string, then dup if present
	if (out->owns_base_url && out->base_url) {
		free((void *)out->base_url);
		out->base_url = NULL;
		out->owns_base_url = 0;
	}
	if (base_url_ptr && base_url_len) {
		char *dup;

		dup = (char *)malloc(base_url_len + 1);
		if (!dup) {
			ret = MCPKG_MC_ERR_NO_MEMORY;
			goto out_r;
		}

		memcpy(dup, base_url_ptr, base_url_len);
		dup[base_url_len] = '\0';
		out->base_url = dup;
		out->owns_base_url = 1;
	}

	out->online = online ? 1 : 0;
	out->flags = flags;

	ret = MCPKG_MC_NO_ERROR;

out_r:
	mcpkg_mp_reader_destroy(&r);
out:
	return ret;
}
