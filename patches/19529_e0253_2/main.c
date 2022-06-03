#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h> // for sigaction
#include <unistd.h> // for syscall

#include <time.h>
#include <sys/time.h>



#include "testcases.h"
typedef unsigned long long ull;


#include <fcntl.h>
#include <sys/mman.h>

void *buff;
unsigned long nr_signals = 0; 

#define SIGBALLOON 		50

#define PAGE_SIZE		(4096)


// clock_t lastResetTime;   
int pagemap, idle;
char * currentPageAddress;
char * idlebitMap, *pageSuggested;
unsigned long  totalPageNo, currentPageNo;
char *pathToIdleMap = "/sys/kernel/mm/page_idle/bitmap"; 

#define GET_Y_BIT_FROM_X(X, Y)(X & ((ull) 1 << Y)) >> Y
#define BYTE_SIZE 8 
#define MASK_TO_GET_PFN 0x007FFFFFFFFFFFFF
#define MAX_BIT 63
#define MAXIMUM_PAGE_SLOT  128 * 1024
#define FILE_WRITE_BYTE 4096
#define FILE_READ_BYTE 8


// phase 2
unsigned long long pagemapBufferSize, idlemapBufferSize;
unsigned long long *pagemapBuffer, *idlemapBuffer;

#define DEBUG 1
#if defined(DEBUG) && DEBUG > 0
    #define PRINTF(fmt, args...) printf("_________USERSIDE:__________ %s:%d:%s(): " fmt, \
  __FILE__, __LINE__, __func__, ##args)
#else
    #define PRINTK(fmt, args...) 
#endif

struct data {
	unsigned long long va;
	int val;
};


struct data *pageData;

////////////////////////////////////////////////


struct timeval lastResetTime, lastUpdateTime; 



void getFileDescriptor();
void resetIdleBitMap();
void updateIdleBitMap();
void throwError(char * message);
void policyHandler();
//////////////////////
void printTimeToExecute();


float timeTaken(struct timeval s, struct timeval e){
    float t = (e.tv_sec - s.tv_sec) + (float)(e.tv_usec - s.tv_usec) / 1000000.0;
    return t;  
}

void printTimeToExecute(struct timeval s, struct timeval e){
    ull totalTime = 1000000 * (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec);
    printf("\n\n time taken : %.3f seconds \n\n", (double) totalTime/1000000);	
    return;
}

#include <pthread.h>

void *update(void *vargp)
{

	while(1) {


        struct timeval currentTime; 
        gettimeofday(&currentTime, NULL);

        if(  timeTaken(lastResetTime, currentTime)  > 20.0 ){
            resetIdleBitMap(); 
        }
        updateIdleBitMap(); 

        // sleep(10); 
	}
}
///////////////////////

void throwError(char * message) {
    perror(message);
    exit(0);
}

void getFileDescriptor() {

    int pid = getpid();
    // printf("pid of current process: %d\n", pid); 
    char path[40] = {};

    sprintf(path, "/proc/%u/pagemap", pid);
    pagemap = open(path, O_RDONLY);
    if (pagemap < 0){
        throwError("Failed to open pagemap.");
    }
    sprintf(path, "/sys/kernel/mm/page_idle/bitmap");
    idle = open(path, O_RDWR);
    if (idle < 0){
        throwError("Failed to open page_idle bitmap.");
    }
}

void closeFiles(){
    close(pagemap);
    close(idle);  
}

// set all bits as 1
void resetIdleBitMap(){

    // struct timeval startTime, endTime;
    // gettimeofday(&startTime, NULL);

    getFileDescriptor(); 
    
    // read usermap directly 
    unsigned long long *p = pagemapBuffer;
    
    // optimized: read this in one go 
	// if (pread(pagemap, p, pagemapBufferSize, 0) != pagemapBufferSize) {
	// 	throwError("resetIdleBitMap(): Failed to read pagemap."); 
	// }


    ull virtualAddr, physicalAddr, pfn, byteWord, offset = 0;
    unsigned long curPageNo = 0;
    char * curPageAddress = buff;

    while (curPageNo < totalPageNo) {
        physicalAddr = p[curPageNo] ; 

        // if (GET_Y_BIT_FROM_X(physicalAddr, MAX_BIT)) { // if present 
            byteWord = 0xff; 
            if (pwrite(idle, & byteWord, BYTE_SIZE, offset) != BYTE_SIZE)
                break; //throwError("\n resetIdleBitMap(): Failed to write \n");

            offset += BYTE_SIZE;
        // }

        curPageNo++;
        curPageAddress += PAGE_SIZE;
    }


    closeFiles(); 
    gettimeofday(&lastResetTime, NULL);


}

void updateIdleBitMap() {

    // struct timeval startTime, endTime;
    // gettimeofday(&startTime, NULL);

    getFileDescriptor(); 


    ull virtualAddr, physicalAddr, pfn, byteWord, offset;
    unsigned long curPageNo = 0;
    char * curPageAddress = buff;

    
    unsigned long long *p;

    // read usermap directly 
    p = pagemapBuffer;
    // optimized: read this in one go 
	if (pread(pagemap, p, pagemapBufferSize, ((ull) curPageAddress/PAGE_SIZE) * BYTE_SIZE ) != pagemapBufferSize ) {
		throwError("updateIdleBitMap(): Failed to read pagemap"); 
	}


    ull totalReadSize = 0, x; 
    p = idlemapBuffer;
	while ((x = read(idle, p, BYTE_SIZE)) > 0) {
		p += BYTE_SIZE;
		totalReadSize += BYTE_SIZE;
	}
	idlemapBufferSize = totalReadSize;
    // printf("totalReadSize : %llu \n ", totalReadSize);

    // read usermap directly 
    p = pagemapBuffer;
    // optimized: read this in one go 
	if (pread(pagemap, p, pagemapBufferSize, ((ull) curPageAddress/PAGE_SIZE) * BYTE_SIZE ) != pagemapBufferSize ) {
		throwError("updateIdleBitMap(): Failed to read pagemap"); 
	}


    ull count1 = 0, count2 = 0, count3 = 0, maxOffset = 0; 
    while (curPageNo < totalPageNo) {
        virtualAddr = (ull) curPageAddress;
        // pageData[curPageNo].va = virtualAddr; 


        physicalAddr = p[curPageNo] ; 



        if (GET_Y_BIT_FROM_X(physicalAddr, MAX_BIT) ) { // if present 
            pfn = physicalAddr & MASK_TO_GET_PFN; // get pfn 
            // byteWord = 0xff; //  1UL << (pfn % 64);
            offset = (pfn / 64) * BYTE_SIZE; 
            if(offset > maxOffset) maxOffset =  offset;
            //        ssize_t pwrite(int fd, void *buf, size_t count, off_t offset);
            // if (pread(idle, & byteWord, BYTE_SIZE, offset) != BYTE_SIZE)
            //     throwError("\n resetIdleBitMap(): Failed to read \n");

            if(offset > idlemapBufferSize) {
                printf("offset: %llu ,idlemapBufferSize : %llu \n", offset , idlemapBufferSize);
                idlebitMap[curPageNo] = -1;
                continue;
            }
            byteWord = idlemapBuffer[offset];


            idlebitMap[curPageNo] = GET_Y_BIT_FROM_X(byteWord, pfn % 64);
            if(idlebitMap[curPageNo] == 0) pageData[curPageNo].val--, count1++; 
            else if(idlebitMap[curPageNo] == 1) pageData[curPageNo].val++, count2++;
            // else if(idlebitMap[curPageNo] > 1) 
        }
        else{
            // page not present
            count3++; 
            idlebitMap[curPageNo] = -1;
            pageData[curPageNo].val = 0;
        }

        curPageNo++;
        curPageAddress += PAGE_SIZE;
    }

    closeFiles(); 

    printf("signal no: %lu, count1 : %llu, count2 : %llu , count3 : %llu \n",nr_signals, count1, count2, count3); 
    // gettimeofday(&lastUpdateTime, NULL);

    
    // gettimeofday(&endTime, NULL);
    // printf("\n updateBitmap(): time taken in policyHandler(): %.3f seconds  \n", timeTaken(startTime, endTime) );

}

 
int comp(const struct data *a, const struct data *b)
{

  // we want sort descending, larger element should come first
  if(a->val < b->val) 
    return 1;
  else 
    return 0;
}

// page replacement policy
void policyHandler(){

    // updateIdleBitMap(); 

    int count = 0, res; 

    unsigned long curPageNo = 0;
    char * curPageAddress = buff;

    int pagesTosuggest = 15 * 1024, i = 0; 
    ull listOfPagesToSwap[ pagesTosuggest ]; 

    //  does not work
    // qsort (pageData, totalPageNo, sizeof(struct data), comp);
    
    // while (curPageNo < totalPageNo) {
    //     if(pageData[curPageNo].val > 0){
    //         listOfPagesToSwap[i++] = pageData[curPageNo].va; 
    //         if(i == pagesTosuggest ) break; 
    //     }
    //     curPageNo++; 
    // }

    // printf("\n before while \n"); 

    while (curPageNo < totalPageNo) {
        if(idlebitMap[curPageNo] == 1){
            if(pageSuggested[curPageNo] == 0){
                pageSuggested[curPageNo] = 1; 
                listOfPagesToSwap[i++] = curPageAddress; 
                if(i == pagesTosuggest ) break; 

            }
        }
        else if(idlebitMap[curPageNo] == 0){
            if(pageSuggested[curPageNo] == 1)
                pageSuggested[curPageNo] = 0; 
        }
        curPageNo++;
        curPageAddress += PAGE_SIZE;
    }


    if(i < pagesTosuggest){
        pagesTosuggest = i;
    }

    if(pagesTosuggest < 128 ) {
        printf("nothing to suggest \n "); 
        return;
    }
    
    printf("Pages suggested for swapping: %d \n", i);
    res = syscall(549, listOfPagesToSwap, pagesTosuggest); 

    // struct timeval startTime, endTime;
    // gettimeofday(&startTime, NULL);


    printf("syscall returned: %d\n", res); 

    // gettimeofday(&endTime, NULL);
    // printf("\n syscall takes (): %.3f seconds  \n", timeTaken(startTime, endTime) );



}

// signal handler
void signalHandler() {

    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);


    printf("\nsignalHandler(): %lu signal receieved\n", ++nr_signals);

    resetIdleBitMap();
    updateIdleBitMap();

    policyHandler(); 
    
    gettimeofday(&endTime, NULL);
    printf("\nsignalHandler(): time taken in policyHandler(): %.3f seconds  \n", timeTaken(startTime, endTime) );




}


int main(int argc, char *argv[])
{
    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);

	int *ptr, nr_pages;

    	ptr = mmap(NULL, TOTAL_MEMORY_SIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	if (ptr == MAP_FAILED) {
		throwError("\n main(): mmap failed \n");
	}
	buff = ptr;
	
	printf("\n\n mmap done ---------------------------------- \n \n"); fflush(stdout);
	memset(buff, 0, TOTAL_MEMORY_SIZE);
	printf("\n \n memset done--------------------------------- \n");  fflush(stdout);



    currentPageNo = 0;
    currentPageAddress = buff;
    totalPageNo = TOTAL_MEMORY_SIZE / PAGE_SIZE;
    printf("total page no : %lu \n", totalPageNo); // fflush(stdout);

    idlebitMap = (char * ) malloc(sizeof(char) * totalPageNo);
    
    pageSuggested = (char * ) malloc(sizeof(char) * totalPageNo);
    pageData = malloc(sizeof(struct data) * totalPageNo);

    int i ; 
    char * curPageAddress = buff;
    for(i = 0; i < totalPageNo; i++){
        pageData[i].val = 10; 
        pageData[i].va = curPageAddress;
        curPageAddress += PAGE_SIZE;
    }
    memset(pageSuggested, 0, sizeof(char) * totalPageNo ); 
    ////////////////////////////////////////////////////////////

    pagemapBufferSize = (8 * totalPageNo);
	pagemapBuffer = malloc(pagemapBufferSize); 


    idlemapBufferSize = (8 * totalPageNo);
	idlemapBuffer = malloc(idlemapBufferSize); 


    resetIdleBitMap(); 
    // sleep(1); 
    updateIdleBitMap();
    




    // pthread_t thread_id;
    // pthread_create(&thread_id, NULL, update, NULL);


    ////////////////////////////////////////////////////////////

        // signal(SIGBALLOON, signalHandler);

	printf("Calling sys_ballooning \n "); // fflush(stdout);

    signal(SIGBALLOON, signalHandler);
    
    int ret = syscall(548);

	if(ret != 0){
	 	throwError("\n main(): sys_ballooning failed \n"); 
	}
    // printf("sys_ballooning returned %d\n", ret); // fflush(stdout);

	 

	// test-case 
	test_case_main(buff, TOTAL_MEMORY_SIZE);


	munmap(ptr, TOTAL_MEMORY_SIZE);
	printf("I received SIGBALLOON %lu times\n", nr_signals); fflush(stdout);

    gettimeofday(&endTime, NULL);
    printf("\n time taken to complete main(): %.3f seconds  \n", timeTaken(startTime, endTime) ); fflush(stdout);
}

