#include <stdio.h>
#include <string.h>
#include "../headers/fs.h"

int c_init_template(const char *dir, const char *name, void *userdata) {
    (void)userdata;

    char main_path[512];
    snprintf(main_path, sizeof(main_path), "%s/main.c", dir);

    if (fs_exists(main_path))
        return 0;

    char content[512];
    snprintf(content, sizeof(content),
        "#include <stdio.h>\n\n"
        "int main(void) {\n"
        "    printf(\"Hello from %s!\\n\");\n"
        "    return 0;\n"
        "}\n", name);

    return fs_write_file(main_path, content);
}

int c_pkg_convert(const char *pkg_path, const char *config_file,
                  void *userdata) {
    (void)userdata;

    char cml[512];
    snprintf(cml, sizeof(cml), "%s/CMakeLists.txt", pkg_path);
    if (!fs_exists(cml))
        return 0;

    char ycfg[512];
    snprintf(ycfg, sizeof(ycfg), "%s/%s", pkg_path, config_file);

    /* use cmake_convert_file from cmake.c */
    extern int cmake_convert_file(const char *, const char *);
    return cmake_convert_file(cml, ycfg);
}
