#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber, TaskManager *taskManager)
:   InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset()),
    taskManager(taskManager) {}

SyscallHandler::~SyscallHandler() {}

void printf(char*);
void printfHex32(uint32_t key);

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp) {
    CPUState* cpuState = (CPUState*)esp;

    // System call numbers taken from https://faculty.nps.edu/cseagle/assembly/sys_call.html (excluding printf and printfHex32)
    switch(cpuState->eax) {
        case 1:
            esp = (uint32_t)taskManager->Exit(cpuState);
            break;
        case 2:
            taskManager->Fork(cpuState);
            break;
        case 4:
            printf((char*)cpuState->ebx);
            break;
        case 5:
            printfHex32(cpuState->ebx);
            break;
        case 7:
            esp = (uint32_t)taskManager->WaitPid(cpuState, cpuState->ebx);
            break;
        case 11:
            esp = (uint32_t)taskManager->Exec(cpuState, (EntryPoint)cpuState->ebx, cpuState->ecx);
            break;
        case 20:
            taskManager->GetPid(cpuState);
            break;
    }

    return esp;
}

void myos::exit() {
    asm ("int $0x80" :: "a" (1));
}

int myos::fork() {
    int ret;
    asm ("int $0x80" : "=a" (ret) : "a" (2));
    return ret;
}

void myos::sysprintf(const char* str) {
    asm ("int $0x80" :: "a" (4), "b" (str));
}

void myos::sysprintfHex32(uint32_t num) {
    asm ("int $0x80" :: "a" (5), "b" (num));
}

int myos::waitpid(int pid) {
    int ret;
    asm ("int $0x80" : "=a" (ret) : "a" (7), "b"(pid));
    return ret;
}

void myos::execve(EntryPoint entryPoint, uint32_t n) {
    asm ("int $0x80" :: "a" (11), "b" (entryPoint), "c" (n));
}

int myos::getpid() {
    int ret;
    asm ("int $0x80" : "=a" (ret) : "a" (20));
    return ret;
}
