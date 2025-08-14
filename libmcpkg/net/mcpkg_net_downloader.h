/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_NET_DOWNLOADER_H
#define MCPKG_NET_DOWNLOADER_H

#include "mcpkg_export.h"

#include <stddef.h>

MCPKG_BEGIN_DECLS

struct McPkgNetClient;
struct McPkgThreadPool;
struct McPkgThreadFuture;

/* Opaque handle */
struct McPkgNetDownloader;
typedef struct McPkgNetDownloader McPkgNetDownloader;

/* Config for the downloader. If pool==NULL, an internal pool is created.
 * parallel/queue only apply when pool==NULL. download_dir is optional; if set,
 * relative 'outfile' paths are resolved against it.
 */
struct McPkgNetDownloaderCfg {
    struct McPkgNetClient   *client;        /* required */
    struct McPkgThreadPool  *pool;          /* optional (borrowed) */
    unsigned int            parallel;       /* default: 4 (when pool==NULL) */
    unsigned int            queue;          /* default: 64 (when pool==NULL) */
    const char              *download_dir;  /* optional base dir */
};

/* Result returned through the future's result pointer on success.
 * Ownership: caller must free(result->outfile) then free(result).
 */
struct McPkgNetDlResult {
    char    *outfile;       /* malloc'd absolute or joined path */
    long    http_code;      /* HTTP status (0 for file:// etc.) */
    size_t  bytes_written;  /* payload size written */
};

/* Lifecycle */
MCPKG_API int  mcpkg_net_downloader_new(const struct McPkgNetDownloaderCfg *cfg,
                                       struct McPkgNetDownloader **out);
MCPKG_API void mcpkg_net_downloader_free(struct McPkgNetDownloader *dl);

/* Enqueue a GET to 'path' (interpreted relative to client's base URL),
 * optional query_kv_pairs (NULL-terminated [k,v,...]), and write to 'outfile'.
 * If 'outfile' is relative and a download_dir was configured, the two are joined.
 *
 * Returns MCPKG_THREAD_* or MCPKG_NET_* style codes synchronously for enqueue errors.
 * The returned future resolves with:
 *   - result: (struct McPkgNetDlResult *) on success
 *   - err:    0 on success; otherwise a MCPKG_NET_ERROR (FS errors mapped to NET IO/NOMEM/etc.)
 */
MCPKG_API int  mcpkg_net_downloader_fetch(struct McPkgNetDownloader *dl,
                                         const char *path,
                                         const char *const *query_kv_pairs,
                                         const char *outfile,
                                         struct McPkgThreadFuture **out_future);

MCPKG_END_DECLS
#endif /* MCPKG_NET_DOWNLOADER_H */
