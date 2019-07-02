#undef CMD_INC_FILE
#define CMD_INC_FILE fadu-nvme

#if !defined(FADU_NVME) || defined(CMD_HEADER_MULTI_READ)
#define FADU_NVME

#include "cmd.h"
#include "plugin.h"

PLUGIN(NAME("fadu", "FADU NVME plugin"),
    COMMAND_LIST(
			ENTRY("shell", "shell command execution", shell_exec)
    )
);

#endif

#include "define_cmd.h"
