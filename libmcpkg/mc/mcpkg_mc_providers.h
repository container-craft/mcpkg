#ifndef MCPKG_MC_PROVIDERS_H
#define MCPKG_MC_PROVIDERS_H

#include <stdint.h>
#include <stddef.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS


typedef enum {
	MCPKG_MC_PROVIDER_MODRINTH   = 1,
	MCPKG_MC_PROVIDER_CURSEFORGE = 2,
	MCPKG_MC_PROVIDER_HANGAR     = 3,
	MCPKG_MC_PROVIDER_LOCAL      = 4,
	MCPKG_MC_PROVIDER_UNKNOWN    = 0x7fff
} MCPKG_MC_PROVIDERS;

// message pack
#define MCPKG_MC_PROVIDER_MP_TAG      "libmcpkg.mc.provider"
#define MCPKG_MC_PROVIDER_MP_VERSION  1
#define MCPKG_MC_PROVIDER_F_ONLINE_REQUIRED	(1u << 0)
#define MCPKG_MC_PROVIDER_F_HAS_API           (1u << 1)
#define MCPKG_MC_PROVIDER_F_PROVIDES_INDEX	(1u << 2)
#define MCPKG_MC_PROVIDER_F_SUPPORTS_CLIENT	(1u << 3)
#define MCPKG_MC_PROVIDER_F_SUPPORTS_SERVER	(1u << 4)
#define MCPKG_MC_PROVIDER_F_SIGNED_METADATA	(1u << 5)

struct McPkgMcProvider;

struct McPkgMcProviderOps {
	int (*init)(struct McPkgMcProvider *p);
	void (*destroy)(struct McPkgMcProvider *p);
	int (*resolve_download_url)(struct McPkgMcProvider *p,
	                            const char *project,
	                            const char *version,
	                            char **out_url);
	int (*fetch_packages_index)(struct McPkgMcProvider *p,
	                            const char *mc_version,
	                            const char *loader,
	                            const char *dest_path);
	int (*is_online)(struct McPkgMcProvider *p);
};

// NOTE: no typedef for struct; PascalCase per your style.
struct McPkgMcProvider {
	MCPKG_MC_PROVIDERS                  provider;
	const char                          *name;          // borrowed
	const char
	*base_url;      // borrowed unless owns_base_url
	int                                 online;         // >0 online, 0 offline
	uint32_t                            flags;          // MCPKG_MC_PROVIDER_F_*
	const struct McPkgMcProviderOps     *ops;
	void                                *priv;          // owned by provider
	uint8_t                             owns_base_url;  // 1 if strdup'ed
};


MCPKG_API int
mcpkg_mc_provider_new(struct McPkgMcProvider **outp, MCPKG_MC_PROVIDERS id);
MCPKG_API void
mcpkg_mc_provider_free(struct McPkgMcProvider *p);

MCPKG_API struct McPkgMcProvider
mcpkg_mc_provider_make(MCPKG_MC_PROVIDERS id);

MCPKG_API struct McPkgMcProvider
mcpkg_mc_provider_from_string_canon(const char *s);

MCPKG_API const char *
mcpkg_mc_provider_to_string(MCPKG_MC_PROVIDERS id);

MCPKG_API MCPKG_MC_PROVIDERS
mcpkg_mc_provider_from_string(const char *s);

MCPKG_API const struct McPkgMcProvider *
mcpkg_mc_provider_table(size_t *count);

MCPKG_API int
mcpkg_mc_provider_is_known(MCPKG_MC_PROVIDERS id);

MCPKG_API int
mcpkg_mc_provider_requires_network(const struct McPkgMcProvider *p);

MCPKG_API int
mcpkg_mc_provider_is_online(struct McPkgMcProvider *p);

MCPKG_API void
mcpkg_mc_provider_set_base_url(struct McPkgMcProvider *p, const char *base_url);

MCPKG_API int
mcpkg_mc_provider_set_base_url_dup(struct McPkgMcProvider *p,
                                   const char *base_url);

MCPKG_API void
mcpkg_mc_provider_set_online(struct McPkgMcProvider *p, int online);

MCPKG_API void
mcpkg_mc_provider_set_ops(struct McPkgMcProvider *p,
                          const struct McPkgMcProviderOps *ops);

// Serialize to a newly allocated buffer (MessagePack only; caller compresses if desired).
// On success: *out_buf points to malloc'd bytes, *out_len set. Return MCPKG_MC_NO_ERROR.
MCPKG_API int
mcpkg_mc_provider_pack(const struct McPkgMcProvider *p,
                       void **out_buf, size_t *out_len);

// Deserialize from a MessagePack buffer into an initialized provider.
// On success, 'out' is filled; base_url may be owned (owns_base_url=1).
MCPKG_API int
mcpkg_mc_provider_unpack(const void *buf, size_t len,
                         struct McPkgMcProvider *out);

MCPKG_END_DECLS
#endif // MCPKG_MC_PROVIDERS_H
