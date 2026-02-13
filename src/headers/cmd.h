#ifndef GOOSE_CMD_H
#define GOOSE_CMD_H

int cmd_init(int argc, char **argv);
int cmd_new(int argc, char **argv);
int cmd_build(int argc, char **argv);
int cmd_run(int argc, char **argv);
int cmd_clean(int argc, char **argv);
int cmd_add(int argc, char **argv);
int cmd_remove(int argc, char **argv);
int cmd_update(int argc, char **argv);
int cmd_test(int argc, char **argv);
int cmd_install(int argc, char **argv);
int cmd_convert(int argc, char **argv);

#endif
