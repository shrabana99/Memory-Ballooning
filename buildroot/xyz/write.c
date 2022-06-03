#include<sys/mman.h> // madvise
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<time.h>
#include<string.h>

int nr_signals = 0;

typedef unsigned long long ull;

#define SIGBALLOON 64

void signal_handler(int sig_num){
	if(sig_num == SIGBALLOON){
		nr_signals++;
		printf("\n SIGBALLOON-%d Recieved",nr_signals);
	}
	else{
		printf("\n Some Other Signal Recieved, SIGNAL NUMBER = %d",sig_num);
	}


	return;
}

#define SIZ 4096 
int main(int argc, char *argv[]){
	signal(SIGBALLOON, signal_handler);
	int syscall_number  = 549;
	printf("\n Calling System Call # %d",syscall_number);

	syscall(548);

	
	int *p, nr_pages;

		printf("doing mmap\n");
    	p = mmap(NULL, SIZ, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

			if (p == MAP_FAILED) {
				printf("not done\n");
			} 
		printf("mmap done, p: %lx\n", p);

		// memset(p, 1, SIZ);
	
	// printf("\n 1111111111111111111111111111111111111111111111111111111\n");

	p[0] = 1;
	p[1] = 2; 
	for(int i = 2; i < SIZ/ sizeof(int); i++){
		p[i] = (p[i-1] + p[i-2]) % 256;
	}

	// printf("\n 22222222222222222222222222222222222222222222222222222222222\n");

	for(int i = 0; i < SIZ/sizeof(int); i++){
		printf("%d ", p[i]);
	}

	printf("\n\n\n");

	printf("\n 444444444444444444444444444444444444444444444444444 \n");
	fflush(stdout); 
	// printf("\n %lu, %lu", p, (void *)p); fflush(stdout);  
	// int res = madvise((void *) p, 1024, MADV_PAGEOUT);

	// printf("res: %d\n", res); 

	ull listOfPagesToSwap[ 1 ]; 
	listOfPagesToSwap[0] = (ull) p;
 
	int res = syscall(549, listOfPagesToSwap, 1);
	printf("res: %d\n", res); 

printf("\n 5555555555555555555555555555555555555555555555555555555555\n");
	// sleep(10);
	// p[0] = 'c';
	for(int i = 0; i < SIZ / sizeof(int); i++){
		printf("%d ", p[i]);
	}

	printf("\n\n\n");

	printf("\n 666666666666666666666666666666666666666666666666666666666666\n");

	munmap(p, SIZ);



	printf("\n Total %d SIGBALLOON Recieved!\n", nr_signals);
	return 0;

}