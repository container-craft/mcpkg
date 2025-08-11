#ifndef TST_FILESYSTEM_H
#define TST_FILESYSTEM_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "mcpkg_fs_std_paths.h"
#include "mcpkg_str_list.h"
#include "tst_macros.h"

#include "fs/mcpkg_fs_error.h"
#include "fs/mcpkg_fs_util.h"
#include "fs/mcpkg_fs_file.h"
#include "fs/mcpkg_fs_dir.h"

#ifdef _WIN32
#  include <windows.h>
#endif

/* ----- tiny tmp helpers ----- */

static char *tmp_root_make(void)
{
#ifdef _WIN32
    char base[MAX_PATH];
    char path[MAX_PATH];
    DWORD n = GetTempPathA((DWORD)sizeof(base), base);
    if (n == 0 || n >= sizeof(base))
        return NULL;
    snprintf(path, sizeof(path), "%smcpkgfs.%u", base,
             (unsigned)GetTickCount());
    if (mcpkg_fs_mkdir_p(path) != MCPKG_FS_OK)
        return NULL;
    return _strdup(path);
#else
    char *tmpl = strdup("/tmp/mcpkgfs.XXXXXX");
    char *p;
    if (!tmpl)
        return NULL;
    p = mkdtemp(tmpl);
    if (!p) {
        free(tmpl);
        return NULL;
    }
    return tmpl; /* mkdtemp mutates */
#endif
}

static char *join2(const char *a, const char *b)
{
    char *out = NULL;
    if (mcpkg_fs_join2(a, b, &out) != MCPKG_FS_OK)
        return NULL;
    return out;
}

static void write_small_file(const char *path, const char *text)
{
    FILE *fp = fopen(path, "wb");
    size_t n = text ? strlen(text) : 0;
    if (!fp)
        return;
    if (n)
        (void)fwrite(text, 1, n, fp);
    fclose(fp);
}

/* ----- tests (existing) ----- */

static void test_join_and_config(void)
{
    char *p = NULL;

    CHECK_OKFS("join2", mcpkg_fs_join2("/a", "b", &p));
    CHECK(strcmp(p, "/a/b") == 0, "joined='%s'", p);
    free(p);

    CHECK_OKFS("config_dir", mcpkg_fs_config_dir(&p));
    CHECK(p && *p, "config_dir non-empty");
    free(p);

    CHECK_OKFS("config_file", mcpkg_fs_config_file(&p));
    CHECK(p && *p, "config_file non-empty");
    free(p);

    {
        char *mods = NULL, *db = NULL;

        CHECK_OKFS("mods_dir",
                   mcpkg_fs_path_mods_dir(&mods, "/root", "fabric",
                                          "foo", "1.0"));
        CHECK(strcmp(mods, "/root/fabric/foo/1.0/mods") == 0,
              "mods='%s'", mods);

        CHECK_OKFS("db_file",
                   mcpkg_fs_path_db_file(&db, "/root", "fabric",
                                         "foo", "1.0"));
        CHECK(strcmp(db, "/root/fabric/foo/1.0/mods/Packages.install")
                  == 0, "db='%s'", db);

        free(mods);
        free(db);
    }
}

static void test_dirs_and_files(void)
{
    char *root = tmp_root_make();
    char *sub = NULL, *deep = NULL, *f1 = NULL, *f2 = NULL;
    unsigned char *buf = NULL;
    size_t sz = 0;

    CHECK(root != NULL, "tmp root created");
    sub  = join2(root, "sub");
    deep = join2(sub,  "deep");
    CHECK(sub && deep, "path alloc");

    CHECK_OKFS("mkdir_p deep", mcpkg_fs_mkdir_p(deep));

    CHECK(mcpkg_fs_dir_exists(deep) == 1, "dir exists");

    f1 = join2(deep, "a.txt");
    CHECK(f1 != NULL, "build file path");

    CHECK_OKFS("touch", mcpkg_fs_touch(f1));
    write_small_file(f1, "hello fs");

    CHECK_OKFS("read_all", mcpkg_fs_read_all(f1, &buf, &sz));
    CHECK_EQ_SZ("Size after read 8:", sz, 8);
    CHECK(memcmp(buf, "hello fs", 8) == 0, "content match");
    free(buf); buf = NULL; sz = 0;

    f2 = join2(sub, "copy.txt");
    CHECK_OKFS("cp_file", mcpkg_fs_cp_file(f1, f2, 1));
    CHECK_OKFS("read copy", mcpkg_fs_read_all(f2, &buf, &sz));
    CHECK_EQ_SZ("Size after copy 8:", sz, 8);
    free(buf); buf = NULL; sz = 0;

#ifndef _WIN32
    {
        char *lnk = join2(sub, "link.txt");
        CHECK_OKFS("ln -sf", mcpkg_fs_ln_sf(f1, lnk, 1));
        free(lnk);
    }
#endif

    CHECK_OKFS("rm_r root", mcpkg_fs_rm_r(root));

    free(f2);
    free(f1);
    free(deep);
    free(sub);
    free(root);
}

#ifndef _WIN32
static void test_cp_dir_rm_r(void)
{
    char *root = tmp_root_make();
    char *src = NULL, *dst = NULL;
    char *s_a = NULL, *s_sub = NULL, *s_b = NULL;
    unsigned char *buf = NULL;
    size_t sz = 0;

    CHECK(root != NULL, "tmp root created");
    src = join2(root, "src");
    dst = join2(root, "dst");
    CHECK(src && dst, "paths");

    CHECK_OKFS("mkdir src", mcpkg_fs_mkdir_p(src));
    s_sub = join2(src, "nested");
    CHECK_OKFS("mkdir nested", mcpkg_fs_mkdir_p(s_sub));

    s_a = join2(src, "a.txt");
    write_small_file(s_a, "AAA");
    s_b = join2(s_sub, "b.txt");
    write_small_file(s_b, "BBB");

    CHECK_OKFS("cp -r", mcpkg_fs_cp_dir(src, dst, 1));

    {
        char *d_a  = join2(dst, "a.txt");
        char *d_b  = join2(dst, "nested");
        char *d_bf = join2(d_b, "b.txt");

        CHECK_OKFS("read a", mcpkg_fs_read_all(d_a, &buf, &sz));
        CHECK_EQ_SZ("Size of A:", sz, 3);
        free(buf); buf = NULL;

        CHECK_OKFS("read b", mcpkg_fs_read_all(d_bf, &buf, &sz));
        CHECK_EQ_SZ("Size of B:", sz, 3);
        free(buf); buf = NULL;

        free(d_bf);
        free(d_b);
        free(d_a);
    }

    CHECK_OKFS("rm -rf root", mcpkg_fs_rm_r(root));

    free(s_b);
    free(s_sub);
    free(s_a);
    free(dst);
    free(src);
    free(root);
}
#endif

/* ----- NEW: explicit /tmp/tmp-test scenario ----- */

#ifndef _WIN32
static void test_explicit_tmp_scenario(void)
{
    const char *base = "/tmp/tmp-test";
    char *file = NULL, *link = NULL, *copy = NULL;
    const char *msg = "mcpkg: hello filesystem\n";
    unsigned char *buf = NULL;
    size_t sz = 0;

    /* start fresh: if it exists, remove just that directory */
    {
        int ex = mcpkg_fs_dir_exists(base);
        CHECK(ex >= 0, "dir_exists returned %d", ex);
        if (ex == 1)
            CHECK_OKFS("rm old base", mcpkg_fs_rm_r(base));
    }

    CHECK_OKFS("mkdir base", mcpkg_fs_mkdir_p(base));

    file = join2(base, "tst.txt");
    CHECK(file != NULL, "path alloc (file)");

    CHECK_OKFS("touch file", mcpkg_fs_touch(file));

    CHECK_OKFS("write_all",
               mcpkg_fs_write_all(file, msg, strlen(msg), 1));

    CHECK_OKFS("read_all", mcpkg_fs_read_all(file, &buf, &sz));
    CHECK_EQ_SZ("Check msg size", sz, strlen(msg));
    CHECK(memcmp(buf, msg, sz) == 0, "content match");
    free(buf); buf = NULL; sz = 0;

    /* symlink + readlink */
    link = join2(base, "tst.txt.link");
    CHECK(link != NULL, "path alloc (link)");
    CHECK_OKFS("ln -sf", mcpkg_fs_ln_sf(file, link, 1));

    {
        char lbuf[PATH_MAX];
        size_t ln = 0;
        CHECK_OKFS("readlink",
                   mcpkg_fs_link_target(link, lbuf, sizeof(lbuf), &ln));
        CHECK(strcmp(lbuf, file) == 0, "link target '%s'", lbuf);
    }

    CHECK_OKFS("unlink link", mcpkg_fs_unlink(link));

    /* copy, then remove copy */
    copy = join2(base, "tst2.txt");
    CHECK(copy != NULL, "path alloc (copy)");
    CHECK_OKFS("cp file", mcpkg_fs_cp_file(file, copy, 1));
    CHECK_OKFS("unlink copy", mcpkg_fs_unlink(copy));

    /* rm dir (removes remaining tst.txt as well) */
    CHECK_OKFS("rm_r base", mcpkg_fs_rm_r(base));

    free(copy);
    free(link);
    free(file);
}
#endif


/* ----- std paths ----- */
static const char *tst_get_expected_home(char *buf, size_t bufsz)
{
#ifdef _WIN32
    const char *p = getenv("USERPROFILE");
    if (p && *p){
        if (strlen(p) < bufsz){
            strcpy(buf, p);
            return buf;
        }
        return NULL;
    }
    {
        const char *drv = getenv("HOMEDRIVE");
        const char *pth = getenv("HOMEPATH");
        if (drv && pth && *drv && *pth){
            size_t need = strlen(drv) + strlen(pth) + 1;
            if (need < bufsz){
                strcpy(buf, drv);
                strcat(buf, pth);
                return buf;
            }
        }
    }
    return NULL;
#else
    const char *h = getenv("HOME");
    if (h && *h){
        if (strlen(h) < bufsz){
            strcpy(buf, h);
            return buf;
        }
        return NULL;
    }
    /* fallback to passwd DB */
#  include <pwd.h>
#  include <sys/types.h>
    {
        struct passwd pw, *res = NULL;
        char tmp[4096];
        if (getpwuid_r(getuid(), &pw, tmp, sizeof tmp, &res) == 0 && res){
            if (pw.pw_dir && *pw.pw_dir && strlen(pw.pw_dir) < bufsz){
                strcpy(buf, pw.pw_dir);
                return buf;
            }
        }
    }
    return NULL;
#endif
}
static void test_std_paths(void)
{
    McPkgStringList *sl;
    char *w = NULL;
    size_t sz;
    const char *p;

    sl = mcpkg_stringlist_new(0, 0);
    CHECK(sl != NULL, "stringlist_new");

    /* TMP */
    CHECK_OKFS("default TMP",
               mcpkg_fs_default_dir(MCPKG_FS_TMP, sl));
    sz = mcpkg_stringlist_size(sl);
    CHECK_EQ_SZ("tmp list size", sz, 1);
    p = mcpkg_stringlist_at(sl, 0);
    CHECK(p && *p, "tmp non-empty");
#ifndef _WIN32
    CHECK_STR("tmp == /tmp", p, "/tmp");
#endif
    CHECK_OKFS("writable TMP",
               mcpkg_fs_writable_dir(MCPKG_FS_TMP, &w));
    CHECK(w && *w, "tmp writable non-empty");
    free(w); w = NULL;

    /* HOME */
    CHECK_OKFS("default HOME",
               mcpkg_fs_default_dir(MCPKG_FS_HOME, sl));
    sz = mcpkg_stringlist_size(sl);
    CHECK_EQ_SZ("home list size", sz, 2);
    p = mcpkg_stringlist_at(sl, 1);
    CHECK(p && *p, "home non-empty");

    {
        char homebuf[4096];
        const char *home = tst_get_expected_home(homebuf, sizeof homebuf);
        /* On dev boxes HOME/USERPROFILE should be present; if not, skip strict compare */
        if (home != NULL){
            CHECK_STR("home at[1]", p, home);
        }
    }

    /* CONFIG */
    CHECK_OKFS("default CONFIG",
               mcpkg_fs_default_dir(MCPKG_FS_CONFIG, sl));
    p = mcpkg_stringlist_at(sl, mcpkg_stringlist_size(sl) - 1);
    CHECK(p && *p, "config non-empty");
#ifndef _WIN32
    CHECK_STR("config == /etc", p, "/etc");
#endif

    /* SHARE */
    CHECK_OKFS("default SHARE",
               mcpkg_fs_default_dir(MCPKG_FS_SHARE, sl));
    p = mcpkg_stringlist_at(sl, mcpkg_stringlist_size(sl) - 1);
    CHECK(p && *p, "share non-empty");
#ifndef _WIN32
    CHECK_STR("share == /usr/share", p, "/usr/share");
#endif

    /* CACHE */
    CHECK_OKFS("default CACHE",
               mcpkg_fs_default_dir(MCPKG_FS_CACHE, sl));
    p = mcpkg_stringlist_at(sl, mcpkg_stringlist_size(sl) - 1);
    CHECK(p && *p, "cache non-empty");
#ifndef _WIN32
    CHECK_STR("cache == /var/cache", p, "/var/cache");
#endif

    mcpkg_stringlist_free(sl);
}


/* ----- entry point ----- */

static inline void run_tst_filesystem(void)
{
    int before = g_tst_fails;

    tst_info("filesystem: starting...");
    test_join_and_config();
    test_dirs_and_files();
#ifndef _WIN32
    test_cp_dir_rm_r();
    test_explicit_tmp_scenario();
#endif
    if (g_tst_fails == before)
        (void)TST_WRITE(TST_OUT_FD, "filesystem: OK\n", 16);

    test_std_paths();
}

#endif /* TST_FILESYSTEM_H */
