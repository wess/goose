#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include "../headers/cmd.h"
#include "../headers/config.h"
#include "../headers/fs.h"
#include "../headers/main.h"
#include "../headers/color.h"

int cmd_init(int argc, char **argv) {
    (void)argc; (void)argv;

    if (fs_exists(GOOSE_CONFIG)) {
        err("%s already exists in this directory", GOOSE_CONFIG);
        return 1;
    }

    char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) {
        err("cannot determine current directory");
        return 1;
    }

    char *name = basename(cwd);

    Config cfg;
    config_default(&cfg, name);

    fs_mkdir(cfg.src_dir);

    char main_path[512];
    snprintf(main_path, sizeof(main_path), "%s/main.c", cfg.src_dir);
    if (!fs_exists(main_path)) {
        char content[512];
        snprintf(content, sizeof(content),
            "#include <stdio.h>\n\n"
            "int main(void) {\n"
            "    printf(\"Hello from %s!\\n\");\n"
            "    return 0;\n"
            "}\n", name);
        fs_write_file(main_path, content);
    }

    config_save(GOOSE_CONFIG, &cfg);
    info("Created", "goose project '%s'", name);
    return 0;
}

int cmd_new(int argc, char **argv) {
    if (argc < 2) {
        err("usage: goose new <name>");
        return 1;
    }

    const char *name = argv[1];

    if (fs_exists(name)) {
        err("directory '%s' already exists", name);
        return 1;
    }

    fs_mkdir(name);

    char path[512];
    snprintf(path, sizeof(path), "%s/src", name);
    fs_mkdir(path);

    Config cfg;
    config_default(&cfg, name);

    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s", name, GOOSE_CONFIG);
    config_save(cfg_path, &cfg);

    snprintf(path, sizeof(path), "%s/src/main.c", name);
    char content[512];
    snprintf(content, sizeof(content),
        "#include <stdio.h>\n\n"
        "int main(void) {\n"
        "    printf(\"Hello from %s!\\n\");\n"
        "    return 0;\n"
        "}\n", name);
    fs_write_file(path, content);

    snprintf(path, sizeof(path), "%s/.gitignore", name);
    fs_write_file(path,
        "builds/\n"
        "packages/\n"
        "goose.lock\n");

    info("Created", "new goose project '%s'", name);
    return 0;
}
