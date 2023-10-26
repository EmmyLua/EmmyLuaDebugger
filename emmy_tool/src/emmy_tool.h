#pragma once

#include "command_line.h"

class EmmyTool {
public:
	explicit EmmyTool(CommandLine &cmd);

	int Launch();

	int Attach();

	int ListProcesses();

	int ArchFile();

	int ArchPid();

	int ReceiveLog();

private:

	CommandLine _cmd;
};

