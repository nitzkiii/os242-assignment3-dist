#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MEM_SIZE 8180

void allocate_and_write_memory(char** mem) {
    *mem = sbrk(0);
    sbrk(MEM_SIZE);
}

void handle_child_process(int s_pid, char* mem) {
    printf("before map: %d\n", sbrk(0));

    char* s_mem = (char*) map_shared_pages(s_pid, getpid(), (uint64)mem, MEM_SIZE);

    if (s_mem == 0) {
        printf("map_shared_pages failed\n");
        exit(1);
    }
    
    strcpy(s_mem, "hello daddy");

    printf("after map: %d\n", sbrk(0));

    unmap_shared_pages(getpid(), (uint64)s_mem, MEM_SIZE);
    printf("after unmap: %d\n", sbrk(0));

    char* s_mem2 = (char*) malloc(MEM_SIZE);
    if (s_mem2 == 0) {
        printf("malloc failed\n");
        exit(1);
    }
    printf("after malloc: %d\n", sbrk(0));
}

int
main(int argc, char *argv[])
{
    char *mem;
    allocate_and_write_memory(&mem);
    
    int s_pid = getpid();
    int pid = fork();

    if (pid == 0) {
        handle_child_process(s_pid, mem);
    }
    else {
        wait(0);

        printf("parent reads from mem: %s\n", mem);
    }

    return 0;;
}