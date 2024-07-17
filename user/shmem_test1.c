#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MEM_SIZE 8180

void allocate_and_write_memory(char** mem) {
    *mem = sbrk(0);
    sbrk(MEM_SIZE);
    strcpy(*mem, "hello child");
}

void handle_child_process(int s_pid, char* mem) {
    char* s_mem = (char*) map_shared_pages(s_pid, getpid(), (uint64)mem, MEM_SIZE);
    if (s_mem == 0) {
        printf("map_shared_pages failed\n");
        exit(1);
    }
    printf("child reads from mem: %s\n", s_mem);
}

int main(int argc, char *argv[]) {
    char *mem;
    allocate_and_write_memory(&mem);

    int s_pid = getpid();
    int pid = fork();

    if (pid == 0) {
        handle_child_process(s_pid, mem);
    } else {
        wait(0);
    }

    exit(0);
}