#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>


#define SHMKEY 314159
#define MAX_PROCS 18

//message struct
struct mesg_buffer {
	long mesg_type;
	char mesg_text[100];
} message;

//message queue variables
int msgid;
key_t key; 

//clock struct
typedef struct{

	unsigned int seconds;
	unsigned int nano_secs;

}sys_clock_t;

//frame table
typedef struct {
	int frames[256];
	int pid[256];	
	int ref_flag[256];
	int dirty_bit[256];
} frame_table_t;

//page table
typedef struct {
	int pages[32];
} page_table_t;

typedef struct {
	int shared_pid_holder[18];   
	int check_proc_num[18];
	int proc_addr_called[18];
	int proc_read_write[18];
	int proc_call_count[18];
} shared_t;

//ALL MY GLOBES
frame_table_t frame_table;        
page_table_t page_table[MAX_PROCS];   
sys_clock_t my_clock;       
int shared_shmid;         
shared_t *shared_shm_ptr; 
int proccesses_running = 0; 
int main_pid_holder[18] = {}; 
int num_forks = 0;
int rand_time_fork[18]; 
int procs_running = 0;
int num_to_log = 0;
int pid;

void getSharedMemory(){
		  
	shared_shmid = shmget(SHMKEY, sizeof(sys_clock_t), IPC_CREAT|0777);

	if(shared_shmid < 0){
		perror("sysClock shmget error in master \n");
		exit(errno);
	}
	
	shared_shm_ptr = shmat(shared_shmid, NULL, 0);

	if(shared_shm_ptr < 0){
		perror("sysClock shmat error in oss\n");
      exit(errno);
  }
}//end sharedmemory

void messageQueueConfig(){

	key = ftok("progfile", 65);
	msgid = msgget(key, 0666 | IPC_CREAT);
}//end message queue

void getForkTimes(){
	
	int i;
   time_t t;
	srand((unsigned) time(&t));

	for(i = 0; i < 18; i++){
		rand_time_fork[i] = (rand() % (500 - 100)) + 100;
	}//end for
}//end fork

#endif


