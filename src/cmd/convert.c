#include <stdio.h>
#include <string.h>
#include "../headers/cmd.h"
#include "../headers/cmake.h"
#include "../headers/config.h"
#include "../headers/main.h"
#include "../headers/color.h"
#include "../headers/fs.h"

int cmd_convert(int argc, char **argv) {
    const char *cmake_path = "CMakeLists.txt";
    const char *yaml_path = GOOSE_CONFIG;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            cmake_path = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            yaml_path = argv[++i];
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

    Config cfg;
    if (cmake_to_config(cmake_path, &cfg) != 0)
        return 1;

    /* show what we extracted */
    info("Name", "%s", cfg.name);
    info("Version", "%s", cfg.version);
    if (strcmp(cfg.src_dir, "src") != 0)
        info("Source", "%s", cfg.src_dir);
    for (int i = 0; i < cfg.include_count; i++)
        info("Include", "%s", cfg.includes[i]);
    if (strlen(cfg.ldflags) > 0)
        info("Ldflags", "%s", cfg.ldflags);

    if (config_save(yaml_path, &cfg) != 0) {
        err("failed to write %s", yaml_path);
        return 1;
    }

    info("Wrote", "%s", yaml_path);
    return 0;
}
