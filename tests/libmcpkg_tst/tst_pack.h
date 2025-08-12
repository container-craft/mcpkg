#ifndef TST_PACK_H
#define TST_PACK_H

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include <pack/mcpkg_msgpack.h>
#include <container/mcpkg_str_list.h>
#include <tst_macros.h>

// Local test schema keys (0/1 reserved for TAG/VER)
#define TPK_K_NUM      2
#define TPK_K_STRLIST  3
#define TPK_K_OPTSTR   4

#define TPK_TAG        "libmcpkg.test.pack"
#define TPK_VER        1

static void test_pack(void)
{
    int ret;
    struct McPkgMpWriter w;
    struct McPkgMpReader r;
    void *buf = NULL;
    size_t len = 0;
    int64_t num = -1;
    int found = 0;
    int ver = 0;
    const char *sp = (const char *)0x1; // sentinel
    size_t slen = 12345;

    // build a sample string list
    McPkgStringList *sl = mcpkg_stringlist_new(0, 0);
    CHECK(sl != NULL, "stringlist_new");

    if (sl) {
        CHECK_OKC("push alpha", mcpkg_stringlist_push(sl, "alpha"));
        CHECK_OKC("push beta",  mcpkg_stringlist_push(sl, "beta"));
        CHECK_OKC("push empty", mcpkg_stringlist_push(sl, ""));
    }

    // writer init
    CHECK_OK_PACK("writer_init", mcpkg_mp_writer_init(&w));

    // 2 header keys + 3 fields
    CHECK_OK_PACK("map_begin", mcpkg_mp_map_begin(&w, 5));

    // header
    CHECK_OK_PACK("write_header",
                  mcpkg_mp_write_header(&w, TPK_TAG, TPK_VER));

    // number
    CHECK_OK_PACK("kv_i32", mcpkg_mp_kv_i32(&w, TPK_K_NUM, 42));

    // strlist
    CHECK_OK_PACK("kv_strlist", mcpkg_mp_kv_strlist(&w, TPK_K_STRLIST, sl));

    // optional string as nil
    CHECK_OK_PACK("kv_nil", mcpkg_mp_kv_nil(&w, TPK_K_OPTSTR));

    // finish → get buffer
    CHECK_OK_PACK("writer_finish", mcpkg_mp_writer_finish(&w, &buf, &len));
    CHECK(buf != NULL && len > 0, "finish produced buffer");

    // destroy writer (caller owns buf now)
    mcpkg_mp_writer_destroy(&w);

    // reader init
    CHECK_OK_PACK("reader_init", mcpkg_mp_reader_init(&r, buf, len));

    // expect tag + version
    CHECK_OK_PACK("expect_tag", mcpkg_mp_expect_tag(&r, TPK_TAG, &ver));
    CHECK_EQ_INT("schema version", ver, TPK_VER);

    // read number
    ret = mcpkg_mp_get_i64(&r, TPK_K_NUM, &num, &found);
    CHECK_OK_PACK("get_i64", ret);
    CHECK(found != 0, "num key found");
    CHECK_EQ_INT("num value", (int)num, 42);

    // read optional string (nil)
    sp = (const char *)0x1; slen = 12345; found = 0;
    ret = mcpkg_mp_get_str_borrow(&r, TPK_K_OPTSTR, &sp, &slen, &found);
    CHECK_OK_PACK("get_str_borrow(nil)", ret);
    CHECK(found != 0, "optstr key found");
    CHECK(sp == NULL && slen == 0, "optstr is NULL/nil");

    // read strlist
    {
        McPkgStringList *out_sl = NULL;
        ret = mcpkg_mp_get_strlist_dup(&r, TPK_K_STRLIST, &out_sl);
        CHECK_OK_PACK("get_strlist_dup", ret);
        CHECK(out_sl != NULL, "strlist unpacked");

        if (out_sl) {
            size_t n = mcpkg_stringlist_size(out_sl);
            CHECK_EQ_SZ("strlist size", n, 3);
            CHECK_STR("strlist[0]", mcpkg_stringlist_at(out_sl, 0), "alpha");
            CHECK_STR("strlist[1]", mcpkg_stringlist_at(out_sl, 1), "beta");
            CHECK_STR("strlist[2]", mcpkg_stringlist_at(out_sl, 2), "");
            mcpkg_stringlist_free(out_sl);
        }
    }

    // cleanup
    mcpkg_mp_reader_destroy(&r);
    if (sl) mcpkg_stringlist_free(sl);
    free(buf);
}

static inline void run_tst_pack(void)
{
    int before = g_tst_fails;

    tst_info("mcpkg message pack's tests: starting...");

    test_pack();

    if (g_tst_fails == before)
        (void)TST_WRITE(TST_OUT_FD,
                         "mcpkg message pack's tests: OK\n",
                         31);
}

#endif // TST_PACK_H
