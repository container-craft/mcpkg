/* SPDX-License-Identifier: MIT */
#ifndef TST_LEDGER_ROUNDTRIP_H
#define TST_LEDGER_ROUNDTRIP_H

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <tst_macros.h>

/* generated model headers */
#include <mp/mcpkg_mp_ledger_hash32.h>
#include <mp/mcpkg_mp_ledger_sth.h>
#include <mp/mcpkg_mp_ledger_tx.h>
#include <mp/mcpkg_mp_ledger_devsig.h>
#include <mp/mcpkg_mp_ledger_devproof.h>
#include <mp/mcpkg_mp_ledger_devlink.h>
#include <mp/mcpkg_mp_ledger_block.h>


/* ---------- tiny helpers ---------- */

static void fill_bin32(unsigned char *dst, unsigned char seed)
{
    size_t i;

    for (i = 0; i < 32; i++)
        dst[i] = (unsigned char)(seed + i);
}

static void fill_bin64(unsigned char *dst, unsigned char seed)
{
    size_t i;

    for (i = 0; i < 64; i++)
        dst[i] = (unsigned char)(seed + i);
}

/* ---------- factories ---------- */

static struct McPkgSTH *mk_sth(void)
{
    struct McPkgSTH *s = mcpkg_mp_ledger_sth_new();

    CHECK_NONNULL("mk_sth alloc", s);
    if (!s)
        return NULL;

    s->size = 100u;
    fill_bin32(s->root, 0x10);
    s->ts_ms = 123456789u;
    s->first = 1u;
    s->last = 100u;
    return s;
}

static struct McPkgTx *mk_tx(void)
{
    struct McPkgTx *t = mcpkg_mp_ledger_tx_new();

    CHECK_NONNULL("mk_tx alloc", t);
    if (!t)
        return NULL;

    fill_bin32(t->from_pub, 0x20);
    fill_bin32(t->to_pub, 0x40);
    t->amount = 5000u;
    t->nonce = 7u;
    fill_bin64(t->sig_from, 0xAA);
    return t;
}

static struct McPkgDevProof *mk_devproof(void)
{
    struct McPkgDevProof *p = mcpkg_mp_ledger_devproof_new();

    CHECK_NONNULL("mk_devproof alloc", p);
    if (!p)
        return NULL;

    p->kind = 3u; /* SIG */
    p->proof_data1 = strdup("sig-type");
    p->proof_data2 = strdup("sig-body");
    fill_bin64(p->proof_sig, 0xCC);
    return p;
}

static struct McPkgDevLink *mk_devlink(void)
{
    struct McPkgDevLink *l = mcpkg_mp_ledger_devlink_new();

    CHECK_NONNULL("mk_devlink alloc", l);
    if (!l)
        return NULL;

    l->provider = strdup("modrinth");
    l->project_id = strdup("P123");
    fill_bin32(l->dev_pub, 0x77);
    l->proof = mk_devproof();
    l->ts_ms = 1712345678;
    return l;
}

static struct McPkgBlock *mk_block(void)
{
    struct McPkgBlock *b = mcpkg_mp_ledger_block_new();

    CHECK_NONNULL("mk_block alloc", b);
    if (!b)
        return NULL;

    b->height = 42u;
    fill_bin32(b->prev, 0x55);
    b->sth = mk_sth();                  /* required by unpack path */
    fill_bin32(b->mint_pub, 0x99);
    fill_bin64(b->sig, 0xAB);
    return b;
}

/* ---------- round-trips ---------- */

static void rt_sth(void)
{
    void *buf = NULL;
    size_t len = 0;
    struct McPkgSTH *in = mk_sth(), *out = NULL;

    CHECK_NONNULL("rt_sth in", in);

    CHECK_OK_PACK("pack sth", mcpkg_mp_ledger_sth_pack(in, &buf, &len));
    CHECK_NONNULL("buf", buf);
    CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

    CHECK_OK_PACK("unpack sth", mcpkg_mp_ledger_sth_unpack(buf, len, &out));
    CHECK_NONNULL("out", out);

    CHECK_EQ_U64("size", out->size, in->size);
    CHECK_EQ_U64("ts_ms", out->ts_ms, in->ts_ms);
    CHECK_EQ_U64("first", out->first, in->first);
    CHECK_EQ_U64("last", out->last, in->last);
    CHECK_MEMEQ("root", out->root, in->root, 32);

    mcpkg_mp_ledger_sth_free(in);
    mcpkg_mp_ledger_sth_free(out);
    free(buf);
}

static void rt_tx(void)
{
    void *buf = NULL;
    size_t len = 0;
    struct McPkgTx *in = mk_tx(), *out = NULL;

    CHECK_NONNULL("rt_tx in", in);

    CHECK_OK_PACK("pack tx", mcpkg_mp_ledger_tx_pack(in, &buf, &len));
    CHECK_NONNULL("buf", buf);
    CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

    CHECK_OK_PACK("unpack tx", mcpkg_mp_ledger_tx_unpack(buf, len, &out));
    CHECK_NONNULL("out", out);

    CHECK_MEMEQ("from_pub", out->from_pub, in->from_pub, 32);
    CHECK_MEMEQ("to_pub", out->to_pub, in->to_pub, 32);
    CHECK_EQ_U64("amount", out->amount, in->amount);
    CHECK_EQ_U64("nonce", out->nonce, in->nonce);
    CHECK_MEMEQ("sig_from", out->sig_from, in->sig_from, 64);

    mcpkg_mp_ledger_tx_free(in);
    mcpkg_mp_ledger_tx_free(out);
    free(buf);
}

static void rt_devlink(void)
{
    void *buf = NULL;
    size_t len = 0;
    struct McPkgDevLink *in = mk_devlink(), *out = NULL;

    CHECK_NONNULL("rt_devlink in", in);

    CHECK_OK_PACK("pack devlink",
                  mcpkg_mp_ledger_devlink_pack(in, &buf, &len));
    CHECK_NONNULL("buf", buf);
    CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

    CHECK_OK_PACK("unpack devlink",
                  mcpkg_mp_ledger_devlink_unpack(buf, len, &out));
    CHECK_NONNULL("out", out);

    CHECK_STR("provider", out->provider, in->provider);
    CHECK_STR("project_id", out->project_id, in->project_id);
    CHECK_MEMEQ("dev_pub", out->dev_pub, in->dev_pub, 32);
    CHECK_NONNULL("proof exists", out->proof);
    if (out->proof && in->proof) {
        CHECK_EQ_U32("proof.kind", out->proof->kind, in->proof->kind);
        CHECK_STR("proof.data1", out->proof->proof_data1, in->proof->proof_data1);
        CHECK_STR("proof.data2", out->proof->proof_data2, in->proof->proof_data2);
        CHECK_MEMEQ("proof.sig", out->proof->proof_sig, in->proof->proof_sig, 64);
    }
    CHECK_EQ_I64("ts_ms", out->ts_ms, in->ts_ms);

    mcpkg_mp_ledger_devlink_free(in);
    mcpkg_mp_ledger_devlink_free(out);
    free(buf);
}

static void rt_block(void)
{
    void *buf = NULL;
    size_t len = 0;
    struct McPkgBlock *in = mk_block(), *out = NULL;

    CHECK_NONNULL("rt_block in", in);

    CHECK_OK_PACK("pack block",
                  mcpkg_mp_ledger_block_pack(in, &buf, &len));
    CHECK_NONNULL("buf", buf);
    CHECK_EQ_SZ("len > 0", len > 0 ? 1 : 0, 1);

    CHECK_OK_PACK("unpack block",
                  mcpkg_mp_ledger_block_unpack(buf, len, &out));
    CHECK_NONNULL("out", out);

    CHECK_EQ_U64("height", out->height, in->height);
    CHECK_MEMEQ("prev", out->prev, in->prev, 32);
    CHECK_MEMEQ("mint_pub", out->mint_pub, in->mint_pub, 32);
    CHECK_MEMEQ("sig", out->sig, in->sig, 64);

    /* nested sth is required in your unpack path */
    CHECK_NONNULL("sth exists", out->sth);
    if (out->sth && in->sth) {
        CHECK_EQ_U64("sth.size", out->sth->size, in->sth->size);
        CHECK_EQ_U64("sth.ts_ms", out->sth->ts_ms, in->sth->ts_ms);
        CHECK_EQ_U64("sth.first", out->sth->first, in->sth->first);
        CHECK_EQ_U64("sth.last", out->sth->last, in->sth->last);
        CHECK_MEMEQ("sth.root", out->sth->root, in->sth->root, 32);
    }

    mcpkg_mp_ledger_block_free(in);
    mcpkg_mp_ledger_block_free(out);
    free(buf);
}

/* ---------- entry point ---------- */

static inline void run_tst_ledger_roundtrip(void)
{
    TST_BLOCK("ledger sth", {
        rt_sth();
    });

    TST_BLOCK("ledger tx", {
        rt_tx();
    });

    TST_BLOCK("ledger devlink", {
        rt_devlink();
    });

    TST_BLOCK("ledger block", {
        rt_block();
    });
}

#endif /* TST_LEDGER_ROUNDTRIP_H */
