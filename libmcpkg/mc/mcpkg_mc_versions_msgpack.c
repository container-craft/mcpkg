#include "mcpkg_mc_versions.h"

#include <stdlib.h>
#include <string.h>
#include "mp/mcpkg_mp_util.h"
#include "mc/mcpkg_mc_util.h"

// Keys: 0/1 reserved for TAG/VER
#define MC_VER_K_CODENAME  2
#define MC_VER_K_SNAPSHOT  3
#define MC_VER_K_VERSIONS  4


int mcpkg_mc_version_family_pack(const struct McPkgMCVersion *v,
                                 void **out_buf, size_t *out_len)
{
	int ret = MCPKG_MC_NO_ERROR;
	struct McPkgMpWriter w;
	int mpret;

	if (!v || !out_buf || !out_len)
		return MCPKG_MC_ERR_INVALID_ARG;

	mpret = mcpkg_mp_writer_init(&w);
	if (mpret != MCPKG_MP_NO_ERROR)
		return mcpkg_mc_err_from_mcpkg_pack_err(mpret);

	// 2 header + 3 fields
	mpret = mcpkg_mp_map_begin(&w, 5);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_write_header(&w, MCPKG_MC_VERSION_FAM_MP_TAG,
	                              MCPKG_MC_VERSION_FAM_MP_VERSION);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_kv_i32(&w, MC_VER_K_CODENAME, (int)v->codename);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_kv_i32(&w, MC_VER_K_SNAPSHOT, v->snapshot ? 1 : 0);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_kv_strlist(&w, MC_VER_K_VERSIONS, v->versions);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_w;
	}

	mpret = mcpkg_mp_writer_finish(&w, out_buf, out_len);
	if (mpret != MCPKG_MP_NO_ERROR)
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);

out_w:
	mcpkg_mp_writer_destroy(&w);
	return ret;
}

int mcpkg_mc_version_family_unpack(const void *buf, size_t len,
                                   struct McPkgMCVersion *out)
{
	int ret = MCPKG_MC_NO_ERROR;
	struct McPkgMpReader r;
	int mpret, ver = 0, found = 0;
	int64_t i64 = 0;
	McPkgStringList *sl = NULL;

	if (!buf || !len || !out)
		return MCPKG_MC_ERR_INVALID_ARG;

	mpret = mcpkg_mp_reader_init(&r, buf, len);
	if (mpret != MCPKG_MP_NO_ERROR)
		return mcpkg_mc_err_from_mcpkg_pack_err(mpret);

	mpret = mcpkg_mp_expect_tag(&r, MCPKG_MC_VERSION_FAM_MP_TAG, &ver);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}

	*out = mcpkg_mc_version_family_make(MCPKG_MC_CODE_NAME_UNKNOWN);

	// codename (required)
	mpret = mcpkg_mp_get_i64(&r, MC_VER_K_CODENAME, &i64, &found);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}

	if (!found) {
		ret = MCPKG_MC_ERR_PARSE;
		goto out_r;
	}
	out->codename = (MCPKG_MC_CODE_NAME)i64;

	// snapshot (optional)
	mpret = mcpkg_mp_get_i64(&r, MC_VER_K_SNAPSHOT, &i64, &found);
	if (mpret == MCPKG_MP_NO_ERROR && found)
		out->snapshot = (i64 != 0) ? 1 : 0;

	// versions (optional -> NULL if absent)
	mpret = mcpkg_mp_get_strlist_dup(&r, MC_VER_K_VERSIONS, &sl);
	if (mpret != MCPKG_MP_NO_ERROR) {
		ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
		goto out_r;
	}
	out->versions = sl; // may be NULL

	ret = MCPKG_MC_NO_ERROR;

out_r:
	mcpkg_mp_reader_destroy(&r);
	return ret;
}
