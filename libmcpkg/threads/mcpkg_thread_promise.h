#ifndef MCPKG_THREAD_PROMISE_H
#define MCPKG_THREAD_PROMISE_H

#include "mcpkg_export.h"
#include "mcpkg_thread_future.h"

MCPKG_BEGIN_DECLS

struct McPkgThreadPromise;		/* opaque */

/* Create a promise + its future.
 * Caller typically gives the future to consumers and keeps the promise.
 * out_p/out_f must be non-NULL.
 */
MCPKG_API int mcpkg_thread_promise_new(struct McPkgThreadPromise **out_p,
                                       struct McPkgThreadFuture **out_f);

/* Free the promise object (does NOT free the future). */
MCPKG_API void mcpkg_thread_promise_free(struct McPkgThreadPromise *p);

/* Get the underlying future (non-owning). */
MCPKG_API struct McPkgThreadFuture *mcpkg_thread_promise_future(
        struct McPkgThreadPromise *p);

/* Complete the promise (sets result+err on the future).
 * Returns 0 or MCPKG_THREAD_E_AGAIN if already set.
 */
MCPKG_API int mcpkg_thread_promise_set(struct McPkgThreadPromise *p,
                                       void *result, int err);

MCPKG_END_DECLS
#endif /* MCPKG_THREAD_PROMISE_H */
