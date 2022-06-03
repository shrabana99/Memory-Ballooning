#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h> // for sigaction
#include <unistd.h> // for syscall

#include <time.h>


#include "testcases.h"
typedef unsigned long long ull;


#include <fcntl.h>
#include <sys/mman.h>

void *buff;
unsigned long nr_signals = 0; 

#define SIGBALLOON 		50

#define PAGE_SIZE		(4096)


clock_t lastResetTime;   
int pagemap, idle;
char * currentPageAddress;
char * idlebitMap;
int * pageScoreArray;  
unsigned long  totalPageNo, currentPageNo;


#define GET_Y_BIT_FROM_X(X, Y)(X & ((ull) 1 << Y)) >> Y
#define BYTE_SIZE 8 
#define MASK_TO_GET_PFN 0x007FFFFFFFFFFFFF
#define MAX_BIT 63
#define MAXIMUM_PAGE_SLOT  128 * 1024

	 
struct timespec startTime, endTime;




void getFileDescriptor();
void resetIdleBitMap();
void updateIdleBitMap();
void throwError(char * message);
void policyHandler();


void throwError(char * message) {
    perror(message);
    exit(0);
}

void getFileDescriptor() {

    int pid = getpid();
    printf("pid of current process: %d\n", pid); 
    char path[40] = {};

    sprintf(path, "/proc/%u/pagemap", pid);
    pagemap = open(path, O_RDONLY);
    if (pagemap < 0)
        throwError("Failed to open /proc/pagemap.");

    sprintf(path, "/sys/kernel/mm/page_idle/bitmap");
    idle = open(path, O_RDWR);
    if (idle < 0)
        throwError("Failed to open sys/kernel/mm/page_idle/bitmap.");

    printf("idlepage bitmap accessed, file descriptor: %d\n", idle); 

}


void resetIdleBitMap(){
    ull virtualAddr, physicalAddr, pfn, byteWord;
    unsigned long curPageNo = 0;
    char * curPageAddress = buff;

    while (curPageNo < totalPageNo) {
        virtualAddr = (ull) curPageAddress;

        if (pread(pagemap, & physicalAddr, BYTE_SIZE, (virtualAddr / PAGE_SIZE) * BYTE_SIZE) != BYTE_SIZE)
            throwError("\n resetIdleBitMap(): Failed to read . \n");

        if (GET_Y_BIT_FROM_X(physicalAddr, MAX_BIT)) {
            pfn = physicalAddr & MASK_TO_GET_PFN;

            byteWord = 1UL << (pfn % 64);
            if (pwrite(idle, & byteWord, BYTE_SIZE, (pfn / 64) * BYTE_SIZE) != BYTE_SIZE)
                throwError("\n resetIdleBitMap(): Failed to write \n");
        }

        curPageNo++;
        curPageAddress += PAGE_SIZE;
    }
    lastResetTime = clock();


}

void updateIdleBitMap() {
    printf("updateIdleBitMap() : Updating the idle page map \n"); 

    ull virtualAddr, physicalAddr, pfn, byteWord;
    unsigned long curPageNo = 0;
    char * curPageAddress = buff;

    while (curPageNo < totalPageNo) {
        virtualAddr = (ull) curPageAddress;

        if (pread(pagemap, & physicalAddr, BYTE_SIZE, (virtualAddr / PAGE_SIZE) * BYTE_SIZE) != BYTE_SIZE)
            throwError("\n updateIdleBitMap(): Failed to read . \n");

        // printf("getbit: %d ", GET_Y_BIT_FROM_X(physicalAddr, MAX_BIT ) );
        if (GET_Y_BIT_FROM_X(physicalAddr, MAX_BIT)) {
            pfn = physicalAddr & MASK_TO_GET_PFN;
            // printf("pfn %lu  ", pfn); 
        
            if (pread(idle, & byteWord, BYTE_SIZE, (pfn / 64) * BYTE_SIZE) != BYTE_SIZE)
                throwError("\n updateIdleBitMap(): Failed to read  \n");
            
            // printf("byteWord: %llu ", byteWord);


            idlebitMap[curPageNo] = GET_Y_BIT_FROM_X(byteWord, pfn % 64);
            //  printf("some output: %d ", GET_Y_BIT_FROM_X(byteWord, pfn % 64) );
        } else {
            idlebitMap[curPageNo] = -1;
        }

        curPageNo++;
        curPageAddress += PAGE_SIZE;
    }
}


// page replacement policy
void policyHandler(){


    updateIdleBitMap(); 

    char*  curPageAddress =  ((char*)buff + TOTAL_MEMORY_SIZE) - PAGE_SIZE;
    int count = 0; 
    // unsigned long curPageNo = totalPageNo - 1; 
    for(int i = totalPageNo-1; i >= 0; i--){
        if(idlebitMap[i] == 1){
            int res = syscall(549, curPageAddress); 
            // printf("res: %d\n", res);
            if(res > 0) count++; 
            curPageAddress -= PAGE_SIZE;
            if(MAXIMUM_PAGE_SLOT == res) break;  
        } 
    }
    printf("Total pages suggested: %d\n", count); 

}

// signal handler
void signalHandler() {

    printf("\nsignalHandler(): %lu signal receieved\n", ++nr_signals);
    policyHandler(); 

    clock_t curr_time = clock();
    double cpu_time_used = ((double)(curr_time - lastResetTime)) / CLOCKS_PER_SEC;
    if (cpu_time_used >= 200 ) {
            resetIdleBitMap(); 
    }

}


int main(int argc, char *argv[])
{
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime);

	int *ptr, nr_pages;

    	ptr = mmap(NULL, TOTAL_MEMORY_SIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	if (ptr == MAP_FAILED) {
		throwError("\n main(): mmap failed \n");
	}
	buff = ptr;
	
	printf("mmap done\n");
	memset(buff, 1, TOTAL_MEMORY_SIZE);
	printf("memset done \n");

    signal(SIGBALLOON, signalHandler);


    currentPageNo = 0;
    currentPageAddress = buff;
    totalPageNo = TOTAL_MEMORY_SIZE / PAGE_SIZE;
    idlebitMap = (char * ) malloc(sizeof(char) * totalPageNo);
    
    pageScoreArray = (char * ) malloc(sizeof(char) * totalPageNo);
    memset(pageScoreArray, 0, sizeof(char) * totalPageNo ); 

    getFileDescriptor();    
    resetIdleBitMap(); 

	printf("Calling sys_ballooning \n "); 
	int ret = syscall(548);
	if(ret != 0){
	 	throwError("\n main(): sys_ballooning failed \n"); 
	}
    printf("sys_ballooning returned %d\n", ret); 

    
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime);

	long timeDifference = (endTime.tv_sec * 1e9 + endTime.tv_nsec ) - (startTime.tv_sec * 1e9 + startTime.tv_nsec);
	printf("\nTime difference between the startTime and endTime = %f ms \n",(float) timeDifference/1000000);
	 

	// test-case 
	test_case_main(buff, TOTAL_MEMORY_SIZE);

	munmap(ptr, TOTAL_MEMORY_SIZE);
	printf("I received SIGBALLOON %lu times\n", nr_signals);
}

