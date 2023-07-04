#include "emmy_tool.h"
#include <string>
#include "utility.h"
#include "Psapi.h"
#include <thread>


void translateText(std::string &text) {
	for (auto &c: text) {
		if (c == '\n') {
			c = ' ';
		}
	}
}

int EmmyTool::Launch() {
	std::string dllDirectory = _cmd.Get<std::string>("dir");
	std::string dllName = _cmd.Get<std::string>("dll");
	std::string workDirectory = _cmd.Get<std::string>("work");
	std::string exeFileName = _cmd.Get<std::string>("exe");
	std::string command = "\"" + exeFileName + "\"" + " " + _cmd.Get<std::string>("args");
	bool createNewWindow = _cmd.Get<bool>("create-new-window");

	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));

	si.cb = sizeof(si);

	ExeInfo info{};
	if (!GetExeInfo(exeFileName.c_str(), info) || info.entryPoint == 0) {
		//MessageEvent("Error: The entry point for the application could not be located", MessageType_Error);
		return -1;
	}

	DWORD flags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;
	if (createNewWindow) {
		flags |= CREATE_NEW_CONSOLE;
	}

	if (!CreateProcess(nullptr,
	                   (LPSTR) command.c_str(),
	                   nullptr,
	                   nullptr,
	                   TRUE,
	                   flags,
	                   nullptr,
	                   workDirectory.c_str(),
	                   &si,
	                   &pi)) {
		//OutputError(GetLastError());
		return -1;
	}

	if (!info.managed) {
		size_t entryPoint = info.entryPoint;

		BYTE breakPointData;
		bool done = false;
		bool hasInject = false;
		bool firstBreakPoint = true;
		bool backToEntry = false;
		while (!done) {
			DEBUG_EVENT debugEvent;
			WaitForDebugEvent(&debugEvent, INFINITE);

			DWORD continueStatus = DBG_EXCEPTION_NOT_HANDLED;

			if (debugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
				if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP ||
				    debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {
					// windows 初始断点忽略
					if (firstBreakPoint) {
						firstBreakPoint = false;
					} else if (!hasInject) {
						if (reinterpret_cast<size_t>(debugEvent.u.Exception.ExceptionRecord.ExceptionAddress) ==
						    entryPoint) {
							CONTEXT context;
							context.ContextFlags = CONTEXT_CONTROL;
							GetThreadContext(pi.hThread, &context);
#ifdef _WIN64
							context.Rip -= 1;
#else
							context.Eip -= 1;
#endif

							SetThreadContext(pi.hThread, &context);

							// Restore the original code bytes.
							SetBreakpoint(pi.hProcess, (LPVOID) entryPoint, false, &breakPointData);
							// done = true;

							// Suspend the thread before we continue the debug event so that the program
							// doesn't continue to run.
							SuspendThread(pi.hThread);

							// 短暂的停止debug
							DebugActiveProcessStop(pi.dwProcessId);

							InjectDllForProcess(pi.hProcess, dllDirectory.c_str(), dllName.c_str());

							// 会将子进程和自己的pid传递给debug session
							std::cout << pi.dwProcessId << std::endl;

							std::string connected;
							std::cin >> connected;

							// 恢复debug
							DebugActiveProcess(pi.dwProcessId);
							ResumeThread(pi.hThread);

							hasInject = true;
							// done = true;
							continueStatus = DBG_CONTINUE;
						}
					} else if (!backToEntry) {
						if (reinterpret_cast<size_t>(debugEvent.u.Exception.ExceptionRecord.ExceptionAddress) ==
						    entryPoint) {
							// 此时重新回归到入口函数
							backToEntry = true;
							done = true;
						}
						continueStatus = DBG_CONTINUE;
					} else {
						// 忽略所有的异常处理，由进程本身的例程处理
						std::cerr << "unhandled C breakPoint at: " << (uint64_t) debugEvent.u.Exception.ExceptionRecord.ExceptionAddress << std::endl;
					}
				} else {
					// 对于非断点异常,第一次交给进程例程处理，第二次输出错误的信息，让进程自己终止
					if (debugEvent.u.Exception.dwFirstChance == 0) {
						switch (debugEvent.u.Exception.ExceptionRecord.ExceptionCode) {
							case EXCEPTION_ACCESS_VIOLATION:
								std::cerr << "The thread tried to read from or write to a virtual address for which it does not have the appropriate access."
										<< std::endl;
								break;
							case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
								std::cerr << "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking."
										<< std::endl;
								break;
							case EXCEPTION_BREAKPOINT:
								std::cerr << "A breakpoint was encountered." << std::endl;
								break;
							case EXCEPTION_DATATYPE_MISALIGNMENT:
								std::cerr << "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on."
										<< std::endl;
								break;
							case EXCEPTION_FLT_DENORMAL_OPERAND:
								std::cerr << "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value."
										<< std::endl;
								break;
							case EXCEPTION_FLT_DIVIDE_BY_ZERO:
								std::cerr << "The thread tried to divide a floating-point value by a floating-point divisor of zero."
										<< std::endl;
								break;
							case EXCEPTION_FLT_INEXACT_RESULT:
								std::cerr << "The result of a floating-point operation cannot be represented exactly as a decimal fraction."
										<< std::endl;
								break;
							case EXCEPTION_FLT_INVALID_OPERATION:
								std::cerr << "This exception represents any floating-point exception not included in this list." << std::endl;
								break;
							case EXCEPTION_FLT_OVERFLOW:
								std::cerr << "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type."
										<< std::endl;
								break;
							case EXCEPTION_FLT_STACK_CHECK:
								std::cerr << "The stack overflowed or underflowed as the result of a floating-point operation." << std::endl;
								break;
							case EXCEPTION_FLT_UNDERFLOW:
								std::cerr << "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type."
										<< std::endl;
								break;
							case EXCEPTION_ILLEGAL_INSTRUCTION:
								std::cerr << "The thread tried to execute an invalid instruction." << std::endl;
								break;
							case EXCEPTION_IN_PAGE_ERROR:
								std::cerr << "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network."
										<< std::endl;
								break;
							case EXCEPTION_INT_DIVIDE_BY_ZERO:
								std::cerr << "The thread tried to divide an integer value by an integer divisor of zero." << std::endl;
								break;
							case EXCEPTION_INT_OVERFLOW:
								std::cerr << "The result of an integer operation caused a carry out of the most significant bit of the result."
										<< std::endl;
								break;
							case EXCEPTION_INVALID_DISPOSITION:
								std::cerr << "An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception."
										<< std::endl;
								break;
							case EXCEPTION_NONCONTINUABLE_EXCEPTION:
								std::cerr << "The thread tried to continue execution after a noncontinuable exception occurred." << std::endl;
								break;
							case EXCEPTION_PRIV_INSTRUCTION:
								std::cerr << "The thread tried to execute an instruction whose operation is not allowed in the current machine mode."
										<< std::endl;
								break;
							case EXCEPTION_SINGLE_STEP:
								std::cerr << "A trace trap or other single-instruction mechanism signaled that one instruction has been executed."
										<< std::endl;
								break;
							default:
								std::cerr << "Unknown type of Structured Exception" << std::endl;
						}

						done = true;
					}
				}
			} else if (debugEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
				done = true;
			} else if (debugEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
				if (!hasInject) {
					// Offset the entry point by the load address of the process.
					entryPoint += reinterpret_cast<size_t>(debugEvent.u.CreateProcessInfo.lpBaseOfImage);
					// Write a break point at the entry point of the application so that we
					// will stop when we reach that point.
					SetBreakpoint(pi.hProcess, reinterpret_cast<void *>(entryPoint), true, &breakPointData);
				}
				CloseHandle(debugEvent.u.CreateProcessInfo.hFile);
			} else if (debugEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
				CloseHandle(debugEvent.u.LoadDll.hFile);
			}
			ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus);
		}
	}
	DebugActiveProcessStop(pi.dwProcessId);
	CloseHandle(pi.hThread);

	// 开一个线程等待stop消息
	std::thread t([&pi]() {
		std::string stop;
		std::cin >> stop;

		TerminateProcess(pi.hProcess, 0);
		CloseHandle(pi.hProcess);
	});
	t.detach();

	//等待进程自然结束
	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD dwExitCode;
	GetExitCodeProcess(pi.hProcess, &dwExitCode);
	std::cerr << "Exit code " << dwExitCode << std::endl;

	CloseHandle(pi.hProcess);

	return 0;
}

int EmmyTool::Attach() {
	const int pid = _cmd.Get<int>("p");
	std::string dir = _cmd.Get<std::string>("dir");
	std::string dll = _cmd.Get<std::string>("dll");
	auto capture = _cmd.Get<bool>("capture-log");
	if (!InjectDll(pid, dir.c_str(), dll.c_str(), capture)) {
		return -1;
	}

	return 0;
}

int EmmyTool::ListProcesses() {
	std::vector<Process> list;
	GetProcesses(list);

	for (auto &value: list) {
		printf("%d\n", value.id);
		// title 中可能出现\n 所以title中的\n全部转为' '
		translateText(value.title);

		printf("%s\n", value.title.c_str());

		translateText(value.path);

		printf("%s\n", value.path.c_str());
		printf("----\n");
	}
	return 0;
}

int EmmyTool::ArchFile() {
	ExeInfo info;
	if (GetExeInfo(_cmd.GetArg(2).c_str(), info)) {
		printf("%d", info.i386);
		return info.i386;
	}
	return -1;//file not exist
}

int EmmyTool::ArchPid() {
	const DWORD processId = std::stoi(_cmd.GetArg(2));
	char fileName[_MAX_PATH];
	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	GetModuleFileNameEx(process, nullptr, fileName, _MAX_PATH);

	ExeInfo info;
	if (GetExeInfo(fileName, info)) {
		printf("%d", info.i386);
		return info.i386;
	}
	return -1;
}

int EmmyTool::ReceiveLog() {
	auto pid = _cmd.Get<DWORD>("p");
	::ReceiveLog(pid);
	return 0;
}
