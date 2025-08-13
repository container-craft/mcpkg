/* SPDX-License-Identifier: MIT
 *
 * pkg/mcpkg_block.h
 * A “block” combines the ledger header with package metadata.
 * - Header: height/prev/CT-style STH/mint key/signature
 * - Meta:   identity, deps, file, origin, etc. (formerly McPkgCache)
 *
 * By default we keep it as COMPOSITION (header + meta). If you prefer a single,
 * flattened struct, set MCPKG_BLOCK_FLAT to 1 before including this header.
 */

#ifndef MCPKG_BLOCK_H
#define MCPKG_BLOCK_H

#include <stdint.h>
#include <stddef.h>
#include "mcpkg_export.h"

/* containers */
#include "container/mcpkg_str_list.h"
#include "container/mcpkg_list.h"

/* crypto + ledger fixed-size types */
#include "crypto/mcpkg_crypto.h"
#include "crypto/mcpkg_crypto_util.h"
#include "crypto/mcpkg_crypto_ledger.h" /* McPkgHash32, McPkgSTH, McPkgPubKey32, McPkgSig64 */

MCPKG_BEGIN_DECLS

/* ---------- enums (as before) ---------- */

typedef enum MCPKG_TRI {
    MCPKG_TRI_UNKNOWN = -1,
    MCPKG_TRI_NO      =  0,
    MCPKG_TRI_YES     =  1
} MCPKG_TRI;

typedef enum MCPKG_DEP_KIND {
    MCPKG_DEP_REQUIRED     = 0,
    MCPKG_DEP_OPTIONAL     = 1,
    MCPKG_DEP_INCOMPATIBLE = 2,
    MCPKG_DEP_EMBEDDED     = 3
} MCPKG_DEP_KIND;

/* ---------- crypto-aware digest ---------- */
/* Provider checksum (algo + lowercase hex). Optional signer pub hint. */
typedef struct McPkgDigest {
    MCPKG_CRYPTO_ALGO_ID provider_algo;  /* MD5/SHA1/SHA256/SHA512/BLAKE2B32 */
    char                 *provider_hex;   /* lowercase hex for provider hash   */
    int                  has_signer_pub; /* 0/1                               */
    McPkgPubKey32        signer_pub;     /* ed25519 pubkey hint (optional)    */
} McPkgDigest;

/* ---------- depends / origin / file ---------- */

typedef struct McPkgDepends {
    char          *id;             /* canonical id (provider-agnostic)           | !NULL */
    char          *version_range;  /* "^1.2.3", ">=1.20,<1.21"                    | !NULL */
    MCPKG_DEP_KIND kind;           /* required/optional/incompatible/embedded     */
    MCPKG_TRI      side;           /* UNKNOWN/YES/NO (client/server bias)         */
} McPkgDepends;

typedef struct McPkgOrigin {
    char *provider;     /* "modrinth" | "curseforge" | "hangar" | ...      | !NULL */
    char *project_id;   /* provider’s project id                             | !NULL */
    char *version_id;   /* provider’s version/file id                        | NULL ok */
    char *source_url;   /* human page URL                                     | NULL ok */
} McPkgOrigin;

typedef struct McPkgFile {
    char    *url;         /* download URL                                    | !NULL */
    char    *file_name;   /* target filename                                  | !NULL */
    uint64_t size;        /* bytes */
    unsigned has_size:1;  /* presence bit */

    McPkgDigest *digests; /* array[N] of provider digests                     | N>=1 recommended */
    size_t       ndigests;
} McPkgFile;

/* ---------- package meta (what used to be McPkgCache) ---------- */

typedef struct McPkg {
    /* identity */
    char *id;           /* canonical id                                      | !NULL */
    char *slug;         /* human slug                                        | NULL ok */
    char *version;      /* resolved version                                   | !NULL */
    char *title;        /* short title                                       | NULL ok */
    char *description;  /* long description                                   | NULL ok */

    /* links/licensing */
    char *license_id;   /* SPDX id if possible                                | NULL ok */
    char *home_page;    /* project home page                                  | NULL ok */
    char *source_repo;  /* VCS URL (if any)                                   | NULL ok */

    /* tags & loaders */
    McPkgStringList *loaders;   /* must contain >=1                            | !NULL */
    McPkgStringList *sections;  /* categories                                   | NULL ok */

    /* config & deps */
    McPkgStringList *configs;   /* shipped configs                               | NULL ok */
    McPkgList       *depends;   /* List<McPkgDepends>                            | NULL ok */

    /* artifact */
    McPkgFile file;      /* file metadata                                      | fields !NULL inside */

    /* sides */
    MCPKG_TRI client;    /* UNKNOWN/NO/YES */
    MCPKG_TRI server;    /* UNKNOWN/NO/YES */

    /* origin (provider linkage) */
    McPkgOrigin origin;

    /* internal flags (future-proof) */
    uint32_t flags;
    uint32_t schema;     /* schema version for (de)serialization */
} McPkg;

/* ---------- ledger header (chain unit) ---------- */

typedef struct McPkgBlockHeader {
    uint64_t     height;     /* 0 = genesis */
    McPkgHash32  prev;       /* hash(prev header); ZERO for genesis */
    McPkgSTH     sth;        /* Signed Tree Head contents */
    McPkgPubKey32 mint_pub;  /* mint’s signing key (ed25519) */
    McPkgSig64    sig;       /* signature over {height,prev,sth,mint_pub} */
} McPkgBlockHeader;

/* ---------- block = header + package meta ---------- */
/* Composition by default. Flip MCPKG_BLOCK_FLAT to flatten fields in-place. */

#ifndef MCPKG_BLOCK_FLAT
typedef struct McPkgBlock {
    McPkgBlockHeader header; /* ledger header */
    McPkg     pkg;    /* package metadata */
} McPkgBlock;
#else
/* Flattened variant (matches your “single struct” sketch). */
typedef struct McPkgBlock {
    /* header */
    uint64_t     height;
    McPkgHash32  prev;
    McPkgSTH     sth;
    McPkgPubKey32 mint_pub;
    McPkgSig64    sig;

    /* meta (flattened) */
    char *id; char *slug; char *version; char *title; char *description;
    char *license_id; char *home_page; char *source_repo;
    McPkgStringList *loaders; McPkgStringList *sections;
    McPkgStringList *configs; McPkgList *depends;
    McPkgFile file;
    MCPKG_TRI client; MCPKG_TRI server;
    McPkgOrigin origin;
    uint32_t flags; uint32_t schema;
} McPkgBlock;
#endif

/* ---------- tiny helpers ---------- */

// /* Is there at least one strong digest on the artifact? */
// static inline int mcpkg_block_has_strong_digest(const McPkgBlock *b)
// {
// #ifndef MCPKG_BLOCK_FLAT
//     const McPkgFile *f = &b->pkg.file;
// #else
//     const McPkgFile *f = &b->file;
// #endif
//     if (!f || !f->digests || f->ndigests == 0) return 0;
//     for (size_t i = 0; i < f->ndigests; i++) {
//         switch (f->digests[i].provider_algo) {
//         case MCPKG_CRYPTO_ALGO_SHA256:
//         case MCPKG_CRYPTO_ALGO_SHA512:
//         case MCPKG_CRYPTO_ALGO_BLAKE2B32:
//             return 1;
//         default: break;
//         }
//     }
//     return 0;
// }

/* Transitional alias: existing code that says “McPkgCache” still compiles. */
typedef struct McPkgBlock McPkgCache;

MCPKG_END_DECLS
#endif /* MCPKG_BLOCK_H */
