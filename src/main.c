#include "lang/c/driver.h"

int main(int argc, char **argv) {
    GooseDriver drv = goose_c_driver();
    return goose_run(argc, argv, &drv);
}
