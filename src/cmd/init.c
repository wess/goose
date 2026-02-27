#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include "../headers/cmd.h"
#include "../headers/config.h"
#include "../headers/framework.h"
#include "../headers/fs.h"
#include "../headers/color.h"

int cmd_init(int argc, char **argv, GooseFramework *fw) {
    (void)argc; (void)argv;

    if (fs_exists(fw->config_file)) {
        err("%s already exists in this directory", fw->config_file);
        return 1;
    }

    char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) {
        err("cannot determine current directory");
        return 1;
    }

    char *name = basename(cwd);

    Config cfg;
    config_default(&cfg, name, fw);

    fs_mkdir(cfg.src_dir);

    /* use consumer init template if available, otherwise write default */
    char main_path[512];
    snprintf(main_path, sizeof(main_path), "%s/%s", cfg.src_dir, fw->init_filename);
    if (!fs_exists(main_path)) {
        if (fw->on_init_template) {
            fw->on_init_template(cfg.src_dir, name, fw->userdata);
        }
    }

    config_save(fw->config_file, &cfg, fw);
    info("Created", "%s project '%s'", fw->tool_name, name);
    return 0;
}

int cmd_new(int argc, char **argv, GooseFramework *fw) {
    if (argc < 2) {
        err("usage: %s new <name>", fw->tool_name);
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
    config_default(&cfg, name, fw);

    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s", name, fw->config_file);
    config_save(cfg_path, &cfg, fw);

    /* use consumer init template */
    if (fw->on_init_template) {
        char src_path[512];
        snprintf(src_path, sizeof(src_path), "%s/src", name);
        fw->on_init_template(src_path, name, fw->userdata);
    }

    snprintf(path, sizeof(path), "%s/.gitignore", name);
    char gitignore[2048];
    snprintf(gitignore, sizeof(gitignore),
        "%s/\n%s/\n%s\n%s",
        fw->build_dir, fw->pkg_dir, fw->lock_file,
        fw->gitignore_extra);
    fs_write_file(path, gitignore);

    info("Created", "new %s project '%s'", fw->tool_name, name);
    return 0;
}
