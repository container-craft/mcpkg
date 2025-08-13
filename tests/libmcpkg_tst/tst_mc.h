#ifndef TST_MC_H
#define TST_MC_H

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <mc/mcpkg_mc.h>
#include <tst_macros.h>

static void test_providers(void)
{
	int rc;
	struct McPkgMcProvider *p = NULL;

	rc = mcpkg_mc_provider_new(&p, MCPKG_MC_PROVIDER_MODRINTH);
	CHECK_OK_MC("provider_new(modrinth)", rc);
	CHECK(p != NULL, "provider alloc");

	CHECK_STR("provider name", p->name, "modrinth");
	CHECK_EQ_INT("requires network",
	             mcpkg_mc_provider_requires_network(p) != 0, 1);

	CHECK_EQ_INT("is_known(MODRINTH)",
	             mcpkg_mc_provider_is_known(MCPKG_MC_PROVIDER_MODRINTH) != 0, 1);

	CHECK_STR("to_string(MODRINTH)",
	          mcpkg_mc_provider_to_string(MCPKG_MC_PROVIDER_MODRINTH),
	          "modrinth");

	mcpkg_mc_provider_free(p);
}


static void test_providers_message_pack(void)
{
	int rc;
	struct McPkgMcProvider *p = NULL;
	void *buf = NULL;
	size_t len = 0;
	struct McPkgMcProvider q; /* unpack target */

	rc = mcpkg_mc_provider_new(&p, MCPKG_MC_PROVIDER_CURSEFORGE);
	CHECK_OK_MC("provider_new(curseforge)", rc);
	CHECK_OK_MC("set_base_url_dup",
	            mcpkg_mc_provider_set_base_url_dup(p,
	                            "https://api.curseforge.com/v1"));

	// no-op but ensures copy/compare path
	p->flags |= 0;

	rc = mcpkg_mc_provider_pack(p, &buf, &len);
	CHECK_OK_MC("provider_pack", rc);
	CHECK(buf != NULL && len > 0, "packed buffer exists");

	memset(&q, 0, sizeof(q));
	rc = mcpkg_mc_provider_unpack(buf, len, &q);
	CHECK_OK_MC("provider_unpack", rc);

	CHECK_EQ_INT("id equal", (int)q.provider, (int)p->provider);
	CHECK_STR("name equal", q.name, p->name);
	CHECK_STR("base_url equal", q.base_url, p->base_url);

	mcpkg_mc_provider_free(p);
	if (q.owns_base_url && q.base_url)
		free((void *)q.base_url);

	free(buf);
}

static void test_loaders(void)
{
	int rc;
	struct McPkgMcLoader *l = NULL;

	rc = mcpkg_mc_loader_new(&l, MCPKG_MC_LOADER_VANILLA);
	CHECK_OK_MC("loader_new(vanilla)", rc);
	CHECK(l != NULL, "loader alloc");

	CHECK_STR("loader name", l->name, "vanilla");
	CHECK_EQ_INT("requires_network", mcpkg_mc_loader_requires_network(l) != 0, 0);

	CHECK_EQ_INT("is_known(VANILLA)",
	             mcpkg_mc_loader_is_known(MCPKG_MC_LOADER_VANILLA) != 0, 1);

	CHECK_STR("to_string(VANILLA)",
	          mcpkg_mc_loader_to_string(MCPKG_MC_LOADER_VANILLA),
	          "vanilla");

	mcpkg_mc_loader_free(l);
}

static void test_loaders_message_pack(void)
{
	int rc;
	struct McPkgMcLoader *l = NULL;
	void *buf = NULL;
	size_t len = 0;
	struct McPkgMcLoader q;

	rc = mcpkg_mc_loader_new(&l, MCPKG_MC_LOADER_FABRIC);
	CHECK_OK_MC("loader_new(fabric)", rc);
	CHECK_OK_MC("set_base_url_dup", mcpkg_mc_loader_set_base_url_dup(l,
	                "https://meta.fabricmc.net"));
	l->flags |= MCPKG_MC_LOADER_F_SUPPORTS_CLIENT;

	rc = mcpkg_mc_loader_pack(l, &buf, &len);
	CHECK_OK_MC("loader_pack", rc);
	CHECK(buf != NULL && len > 0, "packed buffer exists");

	memset(&q, 0, sizeof(q));
	rc = mcpkg_mc_loader_unpack(buf, len, &q);
	CHECK_OK_MC("loader_unpack", rc);

#ifdef TST_COMPILE_FAIL
	CHECK_EQ_INT("id equal", (int)q.loader, (int)LCPKG_MC_LOADER_FABRIC);
#endif

	CHECK_EQ_INT("id equal (safe)", (int)q.loader, (int)l->loader);
	CHECK_STR("name equal", q.name, l->name);
	CHECK_STR("base_url equal", q.base_url, l->base_url);
	CHECK_EQ_INT("flags match", (int)q.flags, (int)l->flags);

	mcpkg_mc_loader_free(l);
	if (q.owns_base_url && q.base_url)
		free((void *)q.base_url);

	free(buf);
}

static void test_versions(void)
{
	int rc;
	struct McPkgMCVersion *vf = NULL;
	const char *latest = NULL;
	const struct McPkgMCVersion *families[1];

	rc = mcpkg_mc_version_family_new(&vf, MCPKG_MC_CODE_NAME_TRICKY_TRIALS);
	CHECK_OK_MC("version_family_new", rc);
	CHECK(vf != NULL, "family alloc");

	// out with the new and in with the old wait ....
	CHECK_OKC("push 1.21.8", mcpkg_stringlist_push(vf->versions, "1.21.8"));
	CHECK_OKC("push 1.21.7", mcpkg_stringlist_push(vf->versions, "1.21.7"));

	rc = mcpkg_mc_version_family_latest(vf, &latest);
	CHECK_EQ_INT("latest present", rc, 1);
	CHECK_STR("latest == 1.21.8", latest, "1.21.8");

	families[0] = vf;
	CHECK_EQ_INT("codename_from_version hits",
	             (int)mcpkg_mc_codename_from_version(families, 1, "1.21.7"),
	             (int)MCPKG_MC_CODE_NAME_TRICKY_TRIALS);

	mcpkg_mc_version_family_free(vf);
}


static void test_versions_message_pack(void)
{
	int rc;
	struct McPkgMCVersion *vf = NULL;
	void *buf = NULL;
	size_t len = 0;
	struct McPkgMCVersion out = mcpkg_mc_version_family_make(
	                                    MCPKG_MC_CODE_NAME_UNKNOWN);


	rc = mcpkg_mc_version_family_new(&vf, MCPKG_MC_CODE_NAME_THE_WILD);
	CHECK_OK_MC("family_new(the_wild)", rc);

	vf->snapshot = 0;
	CHECK_OKC("push 1.19.4", mcpkg_stringlist_push(vf->versions, "1.19.4"));
	CHECK_OKC("push 1.19.3", mcpkg_stringlist_push(vf->versions, "1.19.3"));

	rc = mcpkg_mc_version_family_pack(vf, &buf, &len);
	CHECK_OK_MC("version_family_pack", rc);
	CHECK(buf != NULL && len > 0, "packed buffer exists");

	rc = mcpkg_mc_version_family_unpack(buf, len, &out);
	CHECK_OK_MC("version_family_unpack", rc);

	CHECK_EQ_INT("codename equal", (int)out.codename, (int)vf->codename);
	CHECK_EQ_SZ("versions count", mcpkg_stringlist_size(out.versions),
	            mcpkg_stringlist_size(vf->versions));

	CHECK_STR("v[0] equal", mcpkg_stringlist_at(out.versions, 0),
	          mcpkg_stringlist_at(vf->versions, 0));

	CHECK_STR("v[1] equal", mcpkg_stringlist_at(out.versions, 1),
	          mcpkg_stringlist_at(vf->versions, 1));

	mcpkg_mc_version_family_free(vf);
	if (out.versions)
		mcpkg_stringlist_free(out.versions);

	free(buf);
}

static void test_mc(void)
{
	int rc;
	struct McPkgMc *mc = NULL;
	const char *latest = NULL;
	void *buf = NULL;
	size_t len = 0;

	// create context
	rc = mcpkg_mc_new(&mc);
	CHECK_OK_MC("mc_new", rc);

	// seed registries
	CHECK_OK_MC("seed_providers", mcpkg_mc_seed_providers(mc));
	CHECK_OK_MC("seed_loaders", mcpkg_mc_seed_loaders(mc));
	CHECK_OK_MC("seed_versions_minimal", mcpkg_mc_seed_versions_minimal(mc));

	// find + set current by id
	CHECK_OK_MC("set_current_provider(MODRINTH)",
	            mcpkg_mc_set_current_provider_id(mc, MCPKG_MC_PROVIDER_MODRINTH));
	CHECK_OK_MC("set_current_loader(VANILLA)",
	            mcpkg_mc_set_current_loader_id(mc, MCPKG_MC_LOADER_VANILLA));
	CHECK_OK_MC("set_current_family(TRICKY_TRIALS)",
	            mcpkg_mc_set_current_family_code(mc, MCPKG_MC_CODE_NAME_TRICKY_TRIALS));

	// latest lookup
	rc = mcpkg_mc_latest_for_codename(mc, MCPKG_MC_CODE_NAME_TRICKY_TRIALS,
	                                  &latest);
	CHECK_EQ_INT("latest ok (maybe 1 entry)", rc > -1, 1);
	CHECK(latest != NULL, "latest not null");

	// provider current snapshot round-trip
	CHECK_OK_MC("pack_current_provider",
	            mcpkg_mc_pack_current_provider(mc, &buf, &len));
	CHECK(buf && len > 0, "provider current packed");
	free(buf);
	buf = NULL;
	len = 0;

	// loader current snapshot round-trip
	CHECK_OK_MC("pack_current_loader",
	            mcpkg_mc_pack_current_loader(mc, &buf, &len));
	CHECK(buf && len > 0, "loader current packed");
	free(buf);
	buf = NULL;
	len = 0;

	// family current snapshot round‑trip
	CHECK_OK_MC("pack_current_family",
	            mcpkg_mc_pack_current_family(mc, &buf, &len));
	CHECK(buf && len > 0, "family current packed");

	// This call frees the previous current family internally. Do NOT free it yourself.
	CHECK_OK_MC("unpack_current_family",
	            mcpkg_mc_unpack_current_family(mc, buf, len));

	CHECK(mc->current_version != NULL, "current family set from unpack");

	free(buf);
	buf = NULL;
	len = 0;

	// Submodules
	test_providers();
	test_providers_message_pack();
	test_loaders();
	test_loaders_message_pack();
	test_versions();
	test_versions_message_pack();

	mcpkg_mc_free(mc);
}

static inline void run_tst_mc(void)
{
	int before = g_tst_fails;

	tst_info("mcpkg ninecraft module tests: starting...");
	test_mc();
	if (g_tst_fails == before)
		(void)TST_WRITE(TST_OUT_FD,
		                "mcpkg minecraft module tests: OK\n",
		                31);
}

#endif // TST_MC_H
