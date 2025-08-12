#include <tst_macros.h>

#include "tst_crypto.h"
#include "tst_containers.h"
#include "tst_filesystem.h"
#include "tst_pack.h"
#include "tst_mc.h"

int main(int argc, char **argv){
    tst_init_from_env();

    run_tst_containers();
    run_tst_filesystem();
    run_tst_crypto();
    run_tst_pack();
    run_tst_mc();

    tst_summary("libmcpkg");
    return (g_tst_fails == 0) ? 0 : 1;
}
