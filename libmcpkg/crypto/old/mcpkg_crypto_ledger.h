/* SPDX-License-Identifier: MIT */
/* crypto/mcpkg_crypto_ledger.h — core structs for devlink/delegate/attestation/reward/tx + STH/block */

#ifndef MCPKG_CRYPTO_LEDGER_H
#define MCPKG_CRYPTO_LEDGER_H

#include <stdint.h>
#include <stddef.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

/* ---- fixed sizes ---- */
#define MCPKG_PUBKEY_LEN   32  /* ed25519 public key */
#define MCPKG_SIG_LEN      64  /* ed25519 detached signature */
#define MCPKG_HASH32_LEN   32  /* blake2b-256, sha256, etc. */

/* ---- simple byte arrays ---- */
typedef struct {
	uint8_t b[MCPKG_PUBKEY_LEN];
} McPkgPubKey32;
typedef struct {
	uint8_t b[MCPKG_SIG_LEN];
} McPkgSig64;
typedef struct {
	uint8_t b[MCPKG_HASH32_LEN];
} McPkgHash32;

/* ---- common: package identity reference ---- */
typedef struct McPkgRef {
	char    *pkg_id;     /* owned string, required */
	char    *version;    /* owned string, required */
} McPkgRef;

/* =========================
 *   DEV OWNERSHIP & AUTHN
 * ========================= */

/* Evidence that a dev controls a project */
typedef enum MCPKG_DEV_PROOF_KIND {
	MCPKG_DEV_PROOF_OAUTH = 1,   /* proof_data1 = provider-issued token/id */
	MCPKG_DEV_PROOF_DNS   = 2,   /* proof_data1 = domain, proof_data2 = TXT value */
	MCPKG_DEV_PROOF_SIG   = 3    /* proof_sig present: signature over canonical text */
} MCPKG_DEV_PROOF_KIND;

typedef struct McPkgDevProof {
	MCPKG_DEV_PROOF_KIND kind;
	char    *proof_data1;    /* owned; meaning depends on kind (e.g., oauth id, domain) */
	char    *proof_data2;    /* owned; optional (e.g., TXT content) */
	McPkgSig64 proof_sig;    /* optional; used when kind==SIG */
	int     has_sig;         /* 0/1 */
} McPkgDevProof;

/* DEVLINK: bind provider/project → developer wallet (pubkey) */
typedef struct McPkgDevLink {
	char        *provider;      /* "modrinth", "curseforge", "deb", ... */
	char        *project_id;    /* provider’s project identifier */
	McPkgPubKey32 dev_pub;      /* developer wallet pubkey (ed25519) */
	McPkgDevProof proof;        /* proof of control */
	uint64_t     ts_ms;         /* unix millis (mint may normalize) */
} McPkgDevLink;

/* DELEGATE: dev authorizes a builder CI key for a project/time window */
typedef struct McPkgDelegate {
	McPkgPubKey32 dev_pub;      /* owner */
	McPkgPubKey32 builder_pub;  /* delegate */
	char         *project_id;   /* bound project */
	uint64_t      not_before_ms;
	uint64_t      not_after_ms;
	McPkgSig64    sig_by_dev;   /* signature by dev over the delegation */
} McPkgDelegate;

/* DEVSIG: dev directly signs a manifest (alternative to DELEGATE) */
typedef struct McPkgDevSig {
	McPkgPubKey32 dev_pub;
	McPkgHash32   manifest_sha256;
	McPkgSig64    sig_by_dev;   /* detached sig over raw manifest bytes */
} McPkgDevSig;

/* =========================
 *     ATTESTATION LEAF
 * ========================= */

/* ATT: package attestation (by builder or dev) */
typedef struct McPkgAttestation {
	char         *pkg_id;          /* required */
	char         *version;         /* required */
	McPkgHash32   manifest_sha256; /* hash of manifest.msgpack */
	McPkgPubKey32 signer_pub;      /* who signed the manifest */
	McPkgSig64    signature;       /* detached signature over manifest */
	uint64_t      ts_ms;           /* unix millis (producer or mint) */
} McPkgAttestation;

/* =========================
 *        REWARD / TX
 * ========================= */

typedef struct McPkgReward {
	McPkgPubKey32 to_pub;         /* recipient wallet */
	uint64_t      amount;         /* integer units */
	char         *policy_id;      /* policy/rule id (owned string) */
	uint64_t      att_ref;        /* reference (e.g., leaf index) */
	uint64_t      ts_ms;          /* unix millis */
} McPkgReward;

/* Optional transfer (fungible token model) */
typedef struct McPkgTx {
	McPkgPubKey32 from_pub;
	McPkgPubKey32 to_pub;
	uint64_t      amount;
	uint64_t      nonce;          /* anti-replay */
	McPkgSig64    sig_from;       /* signature over {from,to,amount,nonce} */
} McPkgTx;

/* =========================
 *   REVOCATION / KEY MGMT
 * ========================= */

typedef enum MCPKG_REVOKE_KIND {
	MCPKG_REVOKE_KEY = 1,          /* target = key id (hash of pub) */
	MCPKG_REVOKE_ATTESTATION = 2,  /* target = manifest_sha256 */
	MCPKG_REVOKE_PACKAGE = 3       /* target = packed {pkg_id,version} */
} MCPKG_REVOKE_KIND;

typedef enum MCPKG_REVOKE_REASON {
	MCPKG_REVOKE_COMPROMISE = 1,
	MCPKG_REVOKE_SUPERSEDED = 2,
	MCPKG_REVOKE_POLICY     = 3
} MCPKG_REVOKE_REASON;

typedef struct McPkgRevoke {
	MCPKG_REVOKE_KIND   kind;
	McPkgHash32
	target_hash;  /* when kind=KEY or ATTESTATION; for PACKAGE, pack separately in MP */
	char               *pkg_id;       /* only when kind=PACKAGE */
	char               *version;      /* only when kind=PACKAGE */
	MCPKG_REVOKE_REASON reason;
	uint64_t            ts_ms;
} McPkgRevoke;

/* =========================
 *   MERKLE / PROOFS / STH
 * ========================= */

/* STH: Signed Tree Head (included inside blocks) */
typedef struct McPkgSTH {
	uint64_t    size;        /* number of leaves */
	McPkgHash32 root;        /* Merkle root (blake2b-256) */
	uint64_t    ts_ms;       /* batch close time */
	uint64_t    first;       /* first leaf index in batch */
	uint64_t    last;        /* last leaf index in batch */
} McPkgSTH;

/* Audit path node: sibling hash + side (0=left sibling, 1=right sibling) */
typedef struct McPkgAuditNode {
	McPkgHash32 sibling;
	uint8_t     is_right;    /* 0 or 1 */
} McPkgAuditNode;

typedef struct McPkgAuditPath {
	McPkgAuditNode *nodes;   /* array[n] */
	size_t          count;
} McPkgAuditPath;

/* Consistency proof: list of node hashes (CT algorithm) */
typedef struct McPkgConsistencyProof {
	McPkgHash32 *nodes;      /* array[n] */
	size_t       count;
} McPkgConsistencyProof;

// /* =========================
//  *         BLOCKS
//  * ========================= */
// moved over to mp/*

// } McPkgBlock;

/* =========================
 *      GENESIS CONSTANTS
 * ========================= */

/* 32 bytes of 0x00 */
static const McPkgHash32 MCPKG_ZERO32 = { { 0 } };

/* EMPTY_ROOT_V1 = BLAKE2b-256 of (0x02 || "mcpkg.mint.emptyroot.v1")
 * Precomputed hex: 0eb0d0f20ee2d76c72691627bea2ea7bc4c0c79bb80bddcca363c1cc55a437da
 * You can fill at runtime or keep as constant bytes. */
static const McPkgHash32 MCPKG_EMPTY_ROOT_V1 = {
	{
		0x0e, 0xb0, 0xd0, 0xf2, 0x0e, 0xe2, 0xd7, 0x6c,
		0x72, 0x69, 0x16, 0x27, 0xbe, 0xa2, 0xea, 0x7b,
		0xc4, 0xc0, 0xc7, 0x9b, 0xb8, 0x0b, 0xdd, 0xcc,
		0xa3, 0x63, 0xc1, 0xcc, 0x55, 0xa4, 0x37, 0xda
	}
};

MCPKG_END_DECLS
#endif //MCPKG_LEDGER_H
