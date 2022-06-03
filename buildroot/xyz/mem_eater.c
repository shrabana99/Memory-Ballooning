#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if(argc<2){
        printf("ERROR: Program needs 1 parameter : memory in MB to eat!\n");
    }

    printf("Remember that this program will not terminate and will need to be killed!\n");
    unsigned long mem_to_allocate_mb = atol(argv[1]);
    unsigned long mem_to_allocate_bytes = mem_to_allocate_mb<<20;

    void *mem = mmap(NULL, mem_to_allocate_bytes, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    memset(mem, 0, mem_to_allocate_bytes);

    while(1){
        sleep(100);
    }

    return 0;
}