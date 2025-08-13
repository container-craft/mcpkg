#include <tst_macros.h>

#include "tst_crypto.h"
#include "tst_containers.h"
#include "tst_filesystem.h"
#include "tst_pack.h"
#include "tst_mc.h"
#include "tst_net.h"
#include "tst_ledger_roundtrip.h"
#include "tst_pkg_roundtrip.h"
#include "tst_threads_basic.h"
#include "tst_threads_concurrent.h"

int main(int argc, char **argv)
{
	tst_init_from_env();

	run_tst_containers();
	run_tst_filesystem();
	run_tst_crypto();
	run_tst_pack();
	run_tst_mc();
	run_tst_net();
    run_tst_ledger_roundtrip();
    run_tst_pkg_roundtrip();
    run_threads_basic();
    run_threads_concurrent();

	tst_summary("libmcpkg");
	return (g_tst_fails == 0) ? 0 : 1;
}
