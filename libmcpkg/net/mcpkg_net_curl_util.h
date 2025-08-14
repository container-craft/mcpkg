#ifndef MCPKG_NET_CURL_UTIL_H
#define MCPKG_NET_CURL_UTIL_H

#include <curl/curl.h>
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS


MCPKG_API static struct curl_slist *mcpkg_net_curl_slist_dup(
        const struct curl_slist *in)
{
	const struct curl_slist *p = in;
	struct curl_slist *out = NULL;
	while (p) {
		struct curl_slist *n = curl_slist_append(out, p->data);
		if (!n) {
			curl_slist_free_all(out);
			return NULL;
		}
		out = n;
		p = p->next;
	}
	return out;
}







MCPKG_END_DECLS
#endif // MCPKG_NET_CURL_UTIL_H
