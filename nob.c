#define NOB_IMPLEMENTATION

#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER   "src/"
#define INCLUDE      "include/"

#define PROJECT_NAME "chat"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd,
        "cc", 
        "-Wall", "-Wextra", 
        "-o", BUILD_FOLDER"server", 
        "-I", INCLUDE,
        SRC_FOLDER"server.c");
    if (!nob_cmd_run_sync(cmd)) return 1;

    cmd.count = 0;
    nob_cmd_append(&cmd,
        "cc", 
        "-Wall", "-Wextra", 
        "-o", BUILD_FOLDER"client", 
        "-I", INCLUDE,
        SRC_FOLDER"client.c");
    if (!nob_cmd_run_sync(cmd)) return 1;
}
