#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h> // for sigaction
#include <unistd.h> // for syscall

#include <time.h>


#include "testcases.h"

void *buff;
unsigned long nr_signals = 0;
#define SIGBALLOON 		50

#define PAGE_SIZE		(4096)



// page replacement policy


// signal handler
void signalHandler() {
    printf("%lu signal recieved\n", ++nr_signals);
}


int main(int argc, char *argv[])
{
	int *ptr, nr_pages;

    	ptr = mmap(NULL, TOTAL_MEMORY_SIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	if (ptr == MAP_FAILED) {
		printf("------mmap failed------\n");
       	exit(1);
	}
	buff = ptr;
	
	printf("mmap done\n");
	memset(buff, 0, TOTAL_MEMORY_SIZE);
	printf("memset done \n");


	signal(SIGBALLOON, signalHandler);
	 
	struct timespec startTime;
	struct timespec endTime;


	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime);
	int ret = syscall(548);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime);
	

	
	if(ret != 0){
	 	printf("------syscall failed-----\n");
	 	exit(1);
	}
	else{
		printf("sys_ballooning returned: %d\n", ret);  
	}

	long timeDifference = endTime.tv_nsec - startTime.tv_nsec;
	printf("\n time difference between startTime and endTime = %f ms \n",(float) timeDifference/1000000);
	 

	// test-case 
	test_case_main(buff, TOTAL_MEMORY_SIZE);

	munmap(ptr, TOTAL_MEMORY_SIZE);
	printf("I received SIGBALLOON %lu times\n", nr_signals);
}

