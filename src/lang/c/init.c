#include <stdio.h>
#include <string.h>
#include "../../headers/config.h"
#include "../../headers/fs.h"

int c_init(const Config *cfg, const char *dir, void *ctx) {
    (void)ctx;

    char main_path[512];
    snprintf(main_path, sizeof(main_path), "%s/%s/main.c", dir, cfg->src_dir);

    if (fs_exists(main_path))
        return 0;

    char content[512];
    snprintf(content, sizeof(content),
        "#include <stdio.h>\n\n"
        "int main(void) {\n"
        "    printf(\"Hello from %s!\\n\");\n"
        "    return 0;\n"
        "}\n", cfg->name);
    fs_write_file(main_path, content);
    return 0;
}
