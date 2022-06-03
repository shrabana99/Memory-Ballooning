#include<linux/fs.h>
#include<linux/mm_types.h> // types
#include <linux/sched.h> //  for task_struct
#include <linux/kernel.h>
#include<linux/syscalls.h> //syscall 
#include <linux/file.h> //  file handler 
#include<linux/mm.h> // put page 
#include <linux/namei.h> // directory 
#include <linux/mman.h> // do_madvise
#include<linux/swap.h>
#include<linux/rmap.h> // try_to_unmap

// what for what
#include<linux/sched/signal.h>
// #include<linux/signal_types.h>

// #include <linux/sched/mm.h>
// #include <asm/pgtable.h>



#define DEBUG 0
#if defined(DEBUG) && DEBUG > 0
    #define PRINTK(fmt, args...) printk("_________KERNELSIDE_________ %s:%d:%s(): " fmt, \
  __FILE__, __LINE__, __func__, ##args)
#else
    #define PRINTK(fmt, args...) 
#endif

typedef unsigned long long ull;

#define SOME_MEMORY_LIMIT (1UL * 1024 * 1024)


// numbering signal
#define SIGBALLOON 			50
// size of swapfile
#define SWAP_FILE_SIZE 		512 * 1024 * 1024
// page size in swapfile
#define PAGE_SIZE 4096
// maximum pages present within swapfile 
#define totalPageCount  128 * 1024
// Mode to open file
#define MODE 0666
// used pages within swapfile 
ull totalPageUsed = 0;
// array to keep tracking from which offset the swapfile is empty 
ull swapPageMap[totalPageCount];



// total number of application entering ballooning sub system
int participantNo = 0, firstTimeAllocate = 0, debug = DEBUG;
int sendSignalOnce; 

// application info entering ballooning sub system
struct task_struct *participantTask;

// for disabling swap 
extern int vm_swappiness; 
char fullPath[50] = "/ballooning/swapfile_#\0"; 



void setBallooningState(void); 
void createSwapFile(void); 
int addTOSwapFile(ull addr); 
int callZPR(ull addr); 
int checker(void); 

extern unsigned long shrink_all_memory(unsigned long nr_to_reclaim);

SYSCALL_DEFINE0(ballooning){
	if(debug) {
		PRINTK("debug mode on\n"); 
	}
		
	if(participantNo){
		PRINTK("Already one process has registered\n");
		return -1; 
	}

	setBallooningState(); 
	// printk("current application PID: %d\n",participantTask->pid);
	
	// PRINTK("creating swapfile\n");
	createSwapFile(); 
	// PRINTK("------swapfile created-------\n");
	return 0;
}




SYSCALL_DEFINE2(swaplist, unsigned long long * __user, listOfPagesToSwap, int, sizeofList){


	printk("trying to free %d pages", sizeofList); 
	if(sizeofList == 0){
		return 0; 
	}
	
	int res; 
	ull *buffer;
	
	buffer =   kmalloc (sizeof(ull) * sizeofList , GFP_KERNEL); 

	res = copy_from_user(buffer, listOfPagesToSwap, sizeof(ull) * sizeofList ); 
	if(res) { //  Returns number of bytes that could not be copied. On success, this will be zero. 
		return -1; 
	}
	
	ull addr, i;
	for(i = 0; i < sizeofList; i++){
		addr = *(buffer + i); 
		// printk("%lx", addr);
		res = addTOSwapFile(addr);
		if(res != 0) 
			return res; 

		res = callZPR(addr);
		if(res != 0) 
			return res; 

	}
	
	kfree(buffer);

	return checker(); 
}


void setBallooningState(void){

	participantNo += 1;
	participantTask = current;	
	vm_swappiness = 0;
	sendSignalOnce = 0;
	memset(swapPageMap, 0, sizeof(swapPageMap));

	char pidS[4] ; // = "###\0";
	pidS[0] = (char)   ( 48 + participantTask->pid / 100); 
	pidS[1] = (char)  (48 + (participantTask->pid / 10)% 10); 
	pidS[2] = (char)  (48 + participantTask->pid %10); 
	pidS[3] = '\0'; 

	strcat( fullPath, pidS); 
	
}

void createSwapFile(void){
	struct dentry *dentry;
	struct path path , pathE ;

	int flag = kern_path("/ballooning", LOOKUP_FOLLOW, &pathE);

	if(flag){
		dentry = kern_path_create(AT_FDCWD, "/ballooning", &path, LOOKUP_DIRECTORY);
		vfs_mkdir( path.dentry->d_inode, dentry, 0);
		done_path_create(&path, dentry);
	} 

	struct file *swapFile = NULL;

	swapFile = filp_open(fullPath, O_CREAT|O_RDWR, MODE ); 	

	vfs_fallocate(swapFile, 0, 0, SWAP_FILE_SIZE); 
	filp_close(swapFile, current->files);
}

int addTOSwapFile(ull addr){
	// printk(addr); 
	// write into the swapfile
	PRINTK("openning Swapfile \n");
	struct file *swapFile = NULL;
	swapFile = filp_open(fullPath, O_CREAT|O_RDWR, MODE ); 	
	
	PRINTK("Opened Swapfile \n");
	if(totalPageUsed == totalPageCount ){
		PRINTK(" No space available in swapfile!\n");
		return -2;
	}

	// finding empty offset where we can write this data
	int i;
	for(i = 0; i < totalPageCount; i++){
		if(swapPageMap[i] == 0){
			swapPageMap[i] = addr; 
			totalPageUsed++;
			break;
		}
	}

	char* data = (char*) addr;

	PRINTK("writing data\n");
	loff_t offset = PAGE_SIZE * i; 
	int res = kernel_write(swapFile, data, PAGE_SIZE, &offset);
	PRINTK("kernel write retured: %d, offset: %d\n", res, i);
	filp_close(swapFile, current->files);
	return 0; 
}


int callZPR(ull addr){
	struct vm_area_struct *vma = find_vma(current->mm, addr);
	if(vma == NULL || addr >= vma->vm_end){ // invalid address
		PRINTK(" invalid address\n " );
		return -3;
	}
	zap_page_range(vma, addr, PAGE_SIZE); 
	return 0;
}

int checker(void){

	shrink_all_memory(100000); // (3LL << 15)

	ull freeMemory = nr_free_pages() << (2);
	// printk("free memory : %llu", freeMemory);

	// allowing multiple signals ?? 
	sendSignalOnce = 0; 

	if(freeMemory >  SOME_MEMORY_LIMIT) {
		return 0;
	}
	return totalPageUsed; 

}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* 
 *  end[less] experiments 
 */



	// try_to_unmap(page, TTU_BATCH_FLUSH); 
	


	// page table update

	// free the physical page to the buddy all

	// mapping the page pte
	// PRINTK("%lu", pte->pte);
	// *pte= pte_clear_flags(*pte, _PAGE_PRESENT);
	// swapPageMap[i] = pte; 

	// PRINTK("%lu", pte->pte);
	// flush_tlb_all();

	// printk("present bit: %d ", pte_present(*pte)); 
	// printk("none bit: %d ", pte_none(*pte));
	// // printk("present bit: %d ", pte_dirty(pte)); 
	// // printk("present bit: %d ", pte_(pte)); 



	
	

	// //free_unref_page(page);
	// free_swap_cache(page);
	// put_page(page);
	// *pte= pte_clear_flags(*pte, _PAGE_PRESENT);
	// *pte= pte_set_flags(*pte, _PAGE_PROTNONE);

	// printk("present bit: %d ", pte_present(*pte)); 
	// printk("none bit: %d ", pte_none(*pte));
	// printk("dirty bit: %d ", PageDirty(page));
	// printk("inactive bit: %d ",  PageActive(page));


	// PRINTK("printing pte: %lx and ptent : %lx", pte, *pte); 
	// zap_page_range(vma, addr, PAGE_SIZE); 
	// put_page(page);
	// try_to_unmap(page, TTU_BATCH_FLUSH); 

	
	// //flush_tlb_all();
	
	// // struct page ** pagep = &page;
	// // release_pages(pagep,1);

	// PRINTK(" brefore freeing: %lu" , nr_free_pages() );
	
	// deactivate_page(page);
	// int ret_val_madvise = do_madvise(current->mm, addr, PAGE_SIZE, MADV_PAGEOUT); // MADV_FREE

	
	// // PRINTK("returned val: %d", ret_val_madvise);


	// int ret_free_pages = shrink_all_memory();
    // PRINTK("%d\n", ret_free_pages);

	// printk("present bit: %d ", pte_present(*pte)); 
	// printk("none bit: %d ", pte_none(*pte));
	// printk("_PAGE_PROTNONE bit: %d ", pte_protnone(*pte));

	// printk("%d"); 

	// PRINTK("printing the pte: %lx", *pte);


	// int ret = munmapHelper(addr, PAGE_SIZE); 
	// // PRINTK("%d", ret);
	// vunmap(addr); 

	// vm_munmap(addr, PAGE_SIZE); 


// // #include <time.h>
// #include <linux/time.h>


	// struct timespec startTime, endTime;
	// getnstimeofday(&startTime, NULL);
    // // gettimeofday(&startTime, NULL);