#ifndef TST_CONTAINERS_H
#define TST_CONTAINERS_H

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include <tst_macros.h>

#include <container/mcpkg_list.h>
#include <container/mcpkg_str_list.h>
#include <container/mcpkg_hash.h>
#include <container/mcpkg_map.h>


/* ----- list ----- */

static void test_list_basic(void)
{
	McPkgList *lst;
	int v, out = -1;
	size_t i;

	lst = mcpkg_list_new(sizeof(int), NULL, 0, 0);
	CHECK(lst != NULL, "list_new");

	for (i = 0; i < 5; i++) {
		v = (int)(i + 1); //JOSEPH 1
		CHECK_OKC("Push:",  mcpkg_list_push(lst, &v));
	}
	// always is 10 mcpkg_list_size
	CHECK_EQ_SZ("Size of lst", mcpkg_list_size(lst), 5);

	CHECK_OKC("", mcpkg_list_at(lst, 0, &out));
	CHECK_EQ_INT("mcpkg_list_at 0:", out, 1);

	CHECK_OKC("List At", mcpkg_list_at(lst, 4, &out));
	CHECK_EQ_INT("mcpkg_list_at 4:", out, 5);

	CHECK_OKC("Remove at 2", mcpkg_list_remove_at(lst, 2));
	CHECK_EQ_SZ("mcpkg_list_remove_at: 2", mcpkg_list_size(lst), 4);

	CHECK_OKC("Pop List", mcpkg_list_pop(lst, &out));
	CHECK_EQ_INT("Pop List",  out, 5);
	CHECK_EQ_SZ("Size for 3:", mcpkg_list_size(lst), 3);

	mcpkg_list_free(lst);
}

/* ----- string list ----- */

static void test_strlist_basic(void)
{
	McPkgStringList *sl;
	const char *s;
	char *taken = NULL;

	sl = mcpkg_stringlist_new(0, 0);
	CHECK(sl != NULL, "stringlist_new");

	CHECK_OKC("Push Str", mcpkg_stringlist_push(sl, "alpha"));
	CHECK_OKC("Push Str", mcpkg_stringlist_push(sl, "beta"));
	CHECK_EQ_SZ("mcpkg_stringlist_size = 2", mcpkg_stringlist_size(sl), 2);

	s = mcpkg_stringlist_first(sl);
	CHECK(s && strcmp(s, "alpha") == 0, "first='%s'", s ? s : "(null)");

	s = mcpkg_stringlist_last(sl);
	CHECK(s && strcmp(s, "beta") == 0, "last='%s'", s ? s : "(null)");

	CHECK_EQ_INT("mcpkg_stringlist_index_of beta", mcpkg_stringlist_index_of(sl,
	                "beta"), 1);

	CHECK_OKC("ADD AT 1 Str", mcpkg_stringlist_add(sl, 1, "middle"));
	CHECK_EQ_INT("mcpkg_stringlist_index_of middle:", mcpkg_stringlist_index_of(sl,
	                "middle"), 1);
	CHECK_EQ_SZ("mcpkg_stringlist_size for sl", mcpkg_stringlist_size(sl), 3);

	CHECK_OKC("Pop Str", mcpkg_stringlist_pop(sl, &taken));
	CHECK(taken && strcmp(taken, "beta") == 0, "taken='%s'",
	      taken ? taken : "(null)");
	free(taken);

	mcpkg_stringlist_free(sl);
}

/* ----- hash ----- */

static void test_hash_basic(void)
{
	McPkgHash *h = mcpkg_hash_new(sizeof(int), NULL, 0, 0);
	int v, out = -1;
	size_t it = 0;
	const char *k;
	int count = 0;

	CHECK(h != NULL, "hash_new ok");

	v = 10;
	CHECK_OKC("set a=10", mcpkg_hash_set(h, "a", &v));

	v = 20;
	CHECK_OKC("set b=20", mcpkg_hash_set(h, "b", &v));

	CHECK_EQ_SZ("size after 2 inserts", mcpkg_hash_size(h), 2);

	CHECK_OKC("get a", mcpkg_hash_get(h, "a", &out));
	CHECK_EQ_INT("a == 10", out, 10);

	v = 15;
	CHECK_OKC("update a=15", mcpkg_hash_set(h, "a", &v));
	CHECK_OKC("get a again", mcpkg_hash_get(h, "a", &out));
	CHECK_EQ_INT("a == 15", out, 15);

	CHECK(mcpkg_hash_contains(h, "b") == 1, "contains b");

	CHECK_OKC("remove b", mcpkg_hash_remove(h, "b"));
	CHECK(mcpkg_hash_contains(h, "b") == 0, "b removed");
	CHECK_EQ_SZ("size == 1", mcpkg_hash_size(h), 1);

	CHECK_OKC("iter begin", mcpkg_hash_iter_begin(h, &it));
	while (mcpkg_hash_iter_next(h, &it, &k, &out)) {
		CHECK(k != NULL, "iter key non-null");
		CHECK_EQ_INT("the only value is 15", out, 15);
		count++;
	}
	CHECK_EQ_INT("one element iterated", count, 1);

	mcpkg_hash_free(h);
}


/* ----- map (ordered) ----- */

static void test_map_basic(void)
{
	McPkgMap *m = mcpkg_map_new(sizeof(int), NULL, 0, 0);
	int v, out = -1;
	void *it = NULL;
	const char *k;
	int count = 0;

	CHECK(m != NULL, "map_new ok");

	/* insert 3 pairs (values copied by value) */
	v = 30;
	CHECK_OKC("set k3=30", mcpkg_map_set(m, "k3", &v));
	v = 10;
	CHECK_OKC("set k1=10", mcpkg_map_set(m, "k1", &v));
	v = 20;
	CHECK_OKC("set k2=20", mcpkg_map_set(m, "k2", &v));

	CHECK_EQ_SZ("size after 3 inserts", mcpkg_map_size(m), 3);

	/* get */
	CHECK_OKC("get k2", mcpkg_map_get(m, "k2", &out));
	CHECK_EQ_INT("k2 == 20", out, 20);

	/* iteration BEFORE update/remove: order is k1,k2,k3 */
	CHECK_OKC("iter begin", mcpkg_map_iter_begin(m, &it));
	count = 0;
	while (mcpkg_map_iter_next(m, &it, &k, &out)) {
		if (count == 0) {
			CHECK(strcmp(k, "k1") == 0, "iter[0] key k1");
			CHECK_EQ_INT("iter[0] val", out, 10);
		} else if (count == 1) {
			CHECK(strcmp(k, "k2") == 0, "iter[1] key k2");
			CHECK_EQ_INT("iter[1] val", out, 20);
		} else if (count == 2) {
			CHECK(strcmp(k, "k3") == 0, "iter[2] key k3");
			CHECK_EQ_INT("iter[2] val", out, 30);
		}
		count++;
	}
	CHECK_EQ_INT("iterated 3 before changes", count, 3);

	/* update replaces value for existing key (still 1 entry for k2) */
	v = 25;
	CHECK_OKC("update k2=25", mcpkg_map_set(m, "k2", &v));
	CHECK_OKC("get k2 again", mcpkg_map_get(m, "k2", &out));
	CHECK_EQ_INT("k2 == 25", out, 25);
	CHECK_EQ_SZ("size still 3 after replace", mcpkg_map_size(m), 3);

	/* remove */
	CHECK(mcpkg_map_contains(m, "k2") == 1, "contains k2 before remove");
	CHECK_OKC("remove k2", mcpkg_map_remove(m, "k2"));
	CHECK(mcpkg_map_contains(m, "k2") == 0, "k2 removed");
	CHECK_EQ_SZ("size == 2 after remove", mcpkg_map_size(m), 2);

	/* iteration AFTER removal: order is k1,k3 */
	CHECK_OKC("iter begin again", mcpkg_map_iter_begin(m, &it));
	count = 0;
	while (mcpkg_map_iter_next(m, &it, &k, &out)) {
		if (count == 0) {
			CHECK(strcmp(k, "k1") == 0, "iter2[0] key k1");
			CHECK_EQ_INT("iter2[0] val", out, 10);
		} else if (count == 1) {
			CHECK(strcmp(k, "k3") == 0, "iter2[1] key k3");
			CHECK_EQ_INT("iter2[1] val", out, 30);
		}
		count++;
	}
	CHECK_EQ_INT("iterated 2 after remove", count, 2);

	mcpkg_map_free(m);
}


/* ----- entry point for this header ----- */

static inline void run_tst_containers(void)
{
	int before = g_tst_fails;

	tst_info("containers: starting...");
	test_list_basic();
	test_strlist_basic();
	test_hash_basic();
	test_map_basic();

	if (g_tst_fails == before)
		(void)TST_WRITE(TST_OUT_FD, "containers: OK\n", 15);
}

#endif /* TST_CONTAINERS_H */
