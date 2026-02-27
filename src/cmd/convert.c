#include <stdio.h>
#include <string.h>
#include "../headers/cmd.h"
#include "../headers/cmake.h"
#include "../headers/config.h"
#include "../headers/framework.h"
#include "../headers/color.h"
#include "../headers/fs.h"

int cmd_convert(int argc, char **argv, GooseFramework *fw) {
    const char *cmake_path = "CMakeLists.txt";
    const char *yaml_path = fw->config_file;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            cmake_path = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            yaml_path = argv[++i];
        else if (strcmp(argv[i], "--cmake") == 0)
            continue; /* explicit cmake mode (default behavior) */
        else if (argv[i][0] != '-')
            cmake_path = argv[i];
    }

    if (!fs_exists(cmake_path)) {
        err("cannot find %s", cmake_path);
        return 1;
    }

    if (fs_exists(yaml_path)) {
        warn("Warning", "%s already exists, overwriting", yaml_path);
    }

    info("Converting", "%s", cmake_path);

    /* cmake_convert_file writes the file directly with C-specific fields */
    if (cmake_convert_file(cmake_path, yaml_path) != 0) {
        err("failed to convert %s", cmake_path);
        return 1;
    }

    info("Wrote", "%s", yaml_path);
    return 0;
}
