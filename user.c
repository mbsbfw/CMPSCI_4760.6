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

#include "shared_memory.h"

void vars(int);
void wMessage();

int main(int argc, char* argv[]){
	
	char *ptr;
	int table_id = strtol(argv[1], &ptr, 10);
	printf("table_id %d", table_id);

	getSharedMemory();
	messageQueueConfig();
	vars(table_id);
	
	wMessage();
	
	shmdt(shared_shm_ptr); 
}//end main

void vars(int table_id){
	srand(getpid());

	shared_shm_ptr->proc_addr_called[table_id] = (rand() % (31999-1)) + 1;
	int read_write = (rand() % 9);
	if(read_write <= 5){
		shared_shm_ptr->proc_read_write[table_id] = 0;
	}//end if
	else{
		shared_shm_ptr->proc_read_write[table_id] = 1;
	}//end else
	shared_shm_ptr->proc_call_count[table_id]++;
	shared_shm_ptr -> check_proc_num[table_id] = table_id;

}//vars

void wMessage(){
	
	sprintf(message.mesg_text, "%d", getpid());
	message.mesg_type = 1;
	msgsnd(msgid, &message, sizeof(message), 0);
}
