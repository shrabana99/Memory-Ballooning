/* Use 1GB memory */
#define TOTAL_MEMORY_SIZE	(1UL * 1024 * 1024 * 1024)
#define iterCount 50

#include <sys/time.h>

/*
 * Test-case 1
 */
///////////////////////////////////////////////////////////////////////
 #include <unistd.h>
#define OFFSET 13 //this value will be provided during the evaluation
// F(x) simply computes a number based on x that will be 
// used to check for content matching later
#define F(x) ((x+OFFSET)%128)

/*
     * test case to check whether demand paging works correctly
     * and the contents of the buffer are the same as expected
     */
    void test_case_content_check(char *ptr, unsigned long size)
    {
        unsigned long run_idx=1;
        printf("Starting content checking test...\n"); fflush(stdout);
        sleep(2);
        while(1){
            for(unsigned long i=0; i<size; i++){
                if(ptr[i] != F(i)){
                    printf("run_idx %lu \t: ERROR: Content check failed at i= %lu\n", run_idx, i);
                    exit(-1);    
                }
            }
            printf(" run_idx %lu \t: Completed  \n", run_idx);
			fflush(stdout);
            run_idx++;
            sleep(1);
        }
    }

////////////////////////////////////////////////////////////////////


void printContent(int *ptr, int count, long len){
	int i ;
	for(i = len/2-1; i >= 0, count >= 0; i--, count--){
		printf("%d ", ptr[i]);
	}
	printf("\n\n-----------------------------content printed-----------------------\n\n");

}

long test_case_1(int *ptr, long len)
{
	long i;
	int iter, tmp;

	for (iter = 0; iter < iterCount; iter++) {
		for (i = 0; i < len/2; i++) {
			ptr[i] = ptr[i] + 1;
		}
		// printf("\n\n--------------------  iteration took place ------------------------\n\n"); // iter, (double)ts_tot_exec_us/1000000);	// fflush(stdout);

		if(iter == iterCount - 1) printContent(ptr, 400, len); 
	}

	// test_case_content_check(ptr, 200); 
	return 0;
}


/*
 * main entry point for testing use-cases.
 */
void test_case_main(int *ptr, unsigned long size)
{
	long len;
	struct timeval ts_start, ts_end;
	unsigned long long ts_tot_exec_us;

	len = size / sizeof(int);

	test_case_1(ptr, len);
	// printf("Total execution time for testcase 1 : %.3f seconds\n\n", (double)ts_tot_exec_us/1000000);
}
