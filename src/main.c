#include <string.h>
#include "headers/framework.h"
#include "cc/setup.h"

int main(int argc, char **argv) {
    GooseFramework fw;
    memset(&fw, 0, sizeof(fw));
    goose_c_setup(&fw);
    return goose_main(&fw, argc, argv);
}
