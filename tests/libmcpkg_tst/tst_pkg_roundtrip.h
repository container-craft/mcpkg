/* SPDX-License-Identifier: MIT */
#ifndef TST_PKG_ROUNDTRIP_H
#define TST_PKG_ROUNDTRIP_H

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <tst_macros.h>
#include <mp/mcpkg_mp_util.h>

/* generated model headers */
#include <mp/mcpkg_mp_pkg_digest.h>
#include <mp/mcpkg_mp_pkg_file.h>
#include <mp/mcpkg_mp_pkg_depends.h>
#include <mp/mcpkg_mp_pkg_origin.h>
#include <mp/mcpkg_mp_pkg_meta.h>


/* ---------- helpers: deep compare ---------- */

static int eq_str(const char *a, const char *b)
{
	if (!a && !b)
		return 1;
	if (!a || !b)
		return 0;
	return strcmp(a, b) == 0;
}

static int eq_bin32(const unsigned char *a, const unsigned char *b)
{
	return a && b ? memcmp(a, b, 32) == 0 : a == b;
}

static int eq_digest(const struct McPkgDigest *a, const struct McPkgDigest *b)
{
	if (!a || !b)
		return a == b;
	if (a->algo != b->algo)
		return 0;
	return eq_str(a->hex, b->hex);
}

static int eq_digest_list(const struct McPkgList *lst_a,
                          const struct McPkgList *lst_b)
{
	size_t i, na, nb;

	na = mcpkg_list_size(lst_a);
	nb = mcpkg_list_size(lst_b);
	if (na != nb)
		return 0;

	for (i = 0; i < na; i++) {
		struct McPkgDigest *pa = NULL, *pb = NULL;

		CHECK_OKC("list_at(a)", mcpkg_list_at(lst_a, i, &pa));
		CHECK_OKC("list_at(b)", mcpkg_list_at(lst_b, i, &pb));
		if (!eq_digest(pa, pb))
			return 0;
	}
	return 1;
}

static int eq_stringlist(const struct McPkgStringList *a,
                         const struct McPkgStringList *b)
{
	size_t i, na, nb;

	na = mcpkg_stringlist_size(a);
	nb = mcpkg_stringlist_size(b);
	if (na != nb)
		return 0;

	for (i = 0; i < na; i++) {
		if (!eq_str(mcpkg_stringlist_at(a, i),
		            mcpkg_stringlist_at(b, i)))
			return 0;
	}
	return 1;
}

static int eq_depends(const struct McPkgDepends *a,
                      const struct McPkgDepends *b)
{
	if (!a || !b)
		return a == b;
	if (!eq_str(a->id, b->id))
		return 0;
	if (!eq_str(a->version_range, b->version_range))
		return 0;
	if (a->kind != b->kind)
		return 0;
	if (a->side != b->side)
		return 0;
	return 1;
}

static int eq_depends_list(const struct McPkgList *la,
                           const struct McPkgList *lb)
{
	size_t i, na, nb;

	na = mcpkg_list_size(la);
	nb = mcpkg_list_size(lb);
	if (na != nb)
		return 0;

	for (i = 0; i < na; i++) {
		struct McPkgDepends *da = NULL, *db = NULL;

		CHECK_OKC("dep_at(a)", mcpkg_list_at(la, i, &da));
		CHECK_OKC("dep_at(b)", mcpkg_list_at(lb, i, &db));
		if (!eq_depends(da, db))
			return 0;
	}
	return 1;
}

static int eq_origin(const struct McPkgOrigin *a,
                     const struct McPkgOrigin *b)
{
	if (!a || !b)
		return a == b;
	if (!eq_str(a->provider, b->provider))
		return 0;
	if (!eq_str(a->project_id, b->project_id))
		return 0;
	if (!eq_str(a->version_id, b->version_id))
		return 0;
	if (!eq_str(a->source_url, b->source_url))
		return 0;
	return 1;
}

static int eq_file(const struct McPkgFile *a, const struct McPkgFile *b)
{
	if (!a || !b)
		return a == b;
	if (!eq_str(a->url, b->url))
		return 0;
	if (!eq_str(a->file_name, b->file_name))
		return 0;
	if (a->size != b->size)
		return 0;
	return eq_digest_list(a->digests, b->digests);
}

static int eq_meta(const struct McPkgCache *a, const struct McPkgCache *b)
{
	if (!a || !b)
		return a == b;

	if (!eq_str(a->id, b->id) ||
	    !eq_str(a->slug, b->slug) ||
	    !eq_str(a->version, b->version) ||
	    !eq_str(a->title, b->title) ||
	    !eq_str(a->description, b->description) ||
	    !eq_str(a->license_id, b->license_id) ||
	    !eq_str(a->home_page, b->home_page) ||
	    !eq_str(a->source_repo, b->source_repo))
		return 0;

	if (!eq_stringlist(a->loaders, b->loaders))
		return 0;

	if (!eq_stringlist(a->sections, b->sections))
		return 0;

	if (!eq_stringlist(a->configs, b->configs))
		return 0;

	if (!eq_depends_list(a->depends, b->depends))
		return 0;

	if (!eq_file(a->file, b->file))
		return 0;

	if (a->client != b->client || a->server != b->server)
		return 0;

	if ((!!a->origin) != (!!b->origin))
		return 0;
	if (a->origin && !eq_origin(a->origin, b->origin))
		return 0;

	if (a->flags != b->flags || a->schema != b->schema)
		return 0;

	return 1;
}

/* ---------- factories ---------- */

static struct McPkgDigest *mk_digest(const char *hex, uint32_t algo)
{
	struct McPkgDigest *d = mcpkg_mp_pkg_digest_new();

	CHECK_NONNULL("mk_digest alloc", d);
	if (!d)
		return NULL;

	d->algo = algo;
	d->hex = strdup(hex);
	CHECK_NONNULL("mk_digest hex dup", d->hex);
	return d;
}

static struct McPkgList *mk_digest_list_2(void)
{
	struct McPkgList *lst = mcpkg_list_new(sizeof(struct McPkgDigest *),
	                                       NULL, 0, 0);
	struct McPkgDigest *d1 = NULL, *d2 = NULL;

	CHECK_NONNULL("mk_digest_list alloc", lst);
	if (!lst)
		return NULL;

	d1 = mk_digest("012345", 2);
	d2 = mk_digest("abcdef", 3);
	CHECK(d1 && d2, "two digests");

	CHECK_OKC("push d1", mcpkg_list_push(lst, &d1));
	CHECK_OKC("push d2", mcpkg_list_push(lst, &d2));
	return lst;
}

static struct McPkgFile *mk_file_typical(void)
{
	struct McPkgFile *f = mcpkg_mp_pkg_file_new();

	CHECK_NONNULL("mk_file alloc", f);
	if (!f)
		return NULL;

	f->url = strdup("https://example.invalid/mod.jar");
	f->file_name = strdup("mod.jar");
	f->size = 1234567ULL;
	f->digests = mk_digest_list_2();
	CHECK(f->url && f->file_name && f->digests, "mk_file fields");
	return f;
}

static struct McPkgDepends *mk_dep(const char *id, const char *vr,
                                   uint32_t kind, int32_t side)
{
	struct McPkgDepends *d = mcpkg_mp_pkg_depends_new();

	CHECK_NONNULL("mk_dep alloc", d);
	if (!d)
		return NULL;
	d->id = strdup(id);
	d->version_range = strdup(vr);
	d->kind = kind;
	d->side = side;
	CHECK(d->id && d->version_range, "mk_dep strings");
	return d;
}

static struct McPkgList *mk_dep_list_2(void)
{
	struct McPkgList *lst = mcpkg_list_new(sizeof(struct McPkgDepends *),
	                                       NULL, 0, 0);
	struct McPkgDepends *r1 = NULL, *r2 = NULL;

	CHECK_NONNULL("mk_dep_list alloc", lst);
	if (!lst)
		return NULL;

	r1 = mk_dep("fabric-api", ">=0.100.0", 0u, 1);
	r2 = mk_dep("yetanother", "~1.2.3", 1u, 0);
	CHECK(r1 && r2, "two depends");

	CHECK_OKC("push dep1", mcpkg_list_push(lst, &r1));
	CHECK_OKC("push dep2", mcpkg_list_push(lst, &r2));
	return lst;
}

static struct McPkgOrigin *mk_origin(void)
{
	struct McPkgOrigin *o = mcpkg_mp_pkg_origin_new();

	CHECK_NONNULL("mk_origin alloc", o);
	if (!o)
		return NULL;
	o->provider = strdup("modrinth");
	o->project_id = strdup("P1234");
	o->version_id = strdup("V5678");
	o->source_url = strdup("https://modrinth.example/P1234/V5678");
	CHECK(o->provider && o->project_id && o->source_url, "mk_origin str");
	return o;
}

static struct McPkgStringList *mk_strlist3(const char *a, const char *b,
                const char *c)
{
	struct McPkgStringList *sl = mcpkg_stringlist_new(0, 0);

	CHECK_NONNULL("mk_strlist alloc", sl);
	if (!sl)
		return NULL;
	CHECK_OKC("push a", mcpkg_stringlist_push(sl, a));
	CHECK_OKC("push b", mcpkg_stringlist_push(sl, b));
	CHECK_OKC("push c", mcpkg_stringlist_push(sl, c));
	return sl;
}

static struct McPkgCache *mk_meta_maximal(void)
{
	struct McPkgCache *m = mcpkg_mp_pkg_meta_new();

	CHECK_NONNULL("mk_meta alloc", m);
	if (!m)
		return NULL;

	m->id = strdup("com.example:coolmod");
	m->slug = strdup("coolmod");
	m->version = strdup("1.2.3");
	m->title = strdup("Cool Mod");
	m->description = strdup("Cool mod desc");
	m->license_id = strdup("MIT");
	m->home_page = strdup("https://example.invalid");
	m->source_repo = strdup("https://git.example/coolmod");

	m->loaders = mk_strlist3("fabric", "quilt", "neoforge");
	m->sections = mk_strlist3("gameplay", "worldgen", "");
	m->configs = mk_strlist3("server.conf", "client.conf", "both.conf");

	m->depends = mk_dep_list_2();
	m->file = mk_file_typical();
	m->client = 1;	/* YES */
	m->server = 1;	/* YES */
	m->origin = mk_origin();
	m->flags = 0u;
	m->schema = 1u;
	return m;
}

/* ---------- pack/unpack round-trips ---------- */

static void rt_digest(void)
{
	void *buf = NULL;
	size_t len = 0;
	struct McPkgDigest *in = NULL, *out = NULL;

	in = mk_digest("deadbeef", 2);
	CHECK_NONNULL("rt_digest in", in);

	CHECK_OK_PACK("pack digest",
	              mcpkg_mp_pkg_digest_pack(in, &buf, &len));
	CHECK_NONNULL("buf", buf);
	CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

	CHECK_OK_PACK("unpack digest",
	              mcpkg_mp_pkg_digest_unpack(buf, len, &out));
	CHECK(eq_digest(in, out), "digest equal");

	mcpkg_mp_pkg_digest_free(in);
	mcpkg_mp_pkg_digest_free(out);
	free(buf);
}

static void rt_file(void)
{
	void *buf = NULL;
	size_t len = 0;
	struct McPkgFile *in = NULL, *out = NULL;

	in = mk_file_typical();
	CHECK_NONNULL("rt_file in", in);

	CHECK_OK_PACK("pack file", mcpkg_mp_pkg_file_pack(in, &buf, &len));
	CHECK_NONNULL("buf", buf);
	CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

	CHECK_OK_PACK("unpack file",
	              mcpkg_mp_pkg_file_unpack(buf, len, &out));
	CHECK(eq_file(in, out), "file equal");

	mcpkg_mp_pkg_file_free(in);
	mcpkg_mp_pkg_file_free(out);
	free(buf);
}

static void rt_meta(void)
{
	void *buf = NULL;
	size_t len = 0;
	struct McPkgCache *in = NULL, *out = NULL;

	in = mk_meta_maximal();
	CHECK_NONNULL("rt_meta in", in);

	CHECK_OK_PACK("pack meta", mcpkg_mp_pkg_meta_pack(in, &buf, &len));
	CHECK_NONNULL("buf", buf);
	CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

	CHECK_OK_PACK("unpack meta",
	              mcpkg_mp_pkg_meta_unpack(buf, len, &out));
	CHECK(eq_meta(in, out), "meta equal");

	mcpkg_mp_pkg_meta_free(in);
	mcpkg_mp_pkg_meta_free(out);
	free(buf);
}

/* ---------- negative cases ---------- */

static void neg_missing_required_in_file(void)
{
	/* file.url and file_name are required; omit both */
	void *buf = NULL;
	size_t len = 0;
	struct McPkgFile *in = mcpkg_mp_pkg_file_new();
	struct McPkgFile *out = NULL;
	int ret;

	CHECK_NONNULL("neg file alloc", in);

	/* pack succeeds; missing fields are encoded as nil */
	CHECK_OK_PACK("writer_finish (no fields)", mcpkg_mp_pkg_file_pack(in, &buf,
	                &len));
	CHECK_NONNULL("buf", buf);
	CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

	/* unpack should enforce required -> non-zero error, out remains NULL */
	ret = mcpkg_mp_pkg_file_unpack(buf, len, &out);
	CHECK(ret != MCPKG_MP_NO_ERROR, "unpack should fail (required fields)");
	CHECK_NULL("out is NULL on failure", out);

	mcpkg_mp_pkg_file_free(in);
	mcpkg_mp_pkg_file_free(out);
	free(buf);
}

/* ---------- entry point ---------- */

static inline void run_tst_pkg_roundtrip(void)
{
	TST_BLOCK("pkg digest", {
		rt_digest();
	});

	TST_BLOCK("pkg file", {
		rt_file();
	});

	TST_BLOCK("pkg meta", {
		rt_meta();
	});

	TST_BLOCK("pkg negative: missing required in file", {
		neg_missing_required_in_file();
	});
}

#endif /* TST_PKG_ROUNDTRIP_H */
