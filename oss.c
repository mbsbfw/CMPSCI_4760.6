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

#include "shared_memory.h"

void output();
void sigint(int);
void checkMessageQueue();
void cleanMemory(); 
void childProcess();
void checkTermination();
void memMang();
void wLog(int, int, int);
void checkTable(int);
void searchFrameTable(int, int);
void getDirtyBits();
void chanceIt();
void print();

int main(int argc, char* argv[]){

	getSharedMemory();
	messageQueueConfig();
	signal(SIGINT, sigint);
	signal(SIGSEGV, sigint);

	int option;
	int maxProcs;

	while((option = getopt(argc, argv, "hn:")) != -1){

		switch(option){
			case 'h' :
				printf("\t\t---Help Menu---\n");
				printf("\t-h Display Help Menu\n");
				printf("\t-n x indicate the maximum total of child processes\n");
				exit(1);
			case 'n' :
				maxProcs = atoi(optarg);
				if(maxProcs > 20)
					maxProcs = 18;
				break;

			case '?' :
				printf("ERROR: Improper arguments");
				break;
		}//end switch
	}//end while
	
	getForkTimes();
	getDirtyBits();

	while(procs_running == 0){

		childProcess();
		checkMessageQueue();
		memMang();
		checkTermination();
	}//end while

	output();
	cleanMemory();
	return 0;	
}

void output(){
	int i;

	for(i = 0; i < 18; i++){
		printf("%d ", main_pid_holder[i]);
	}//end for
}//end out


void sigint(int x){
	output();

	cleanMemory();
	printf("^C gotcha\n");
	exit(0);
}//end sigint


void childProcess(){
	
	int i;

	for(i = 0; i < 18; i++){
		if(main_pid_holder[i] == 0){
			int pos_pid = i;
			char buffer[10];

			sprintf(buffer, "%d", pos_pid);

			if((main_pid_holder[i] = fork()) == 0){
				execl("./user", "user", buffer, NULL);
			}//end if
			break;
		}//end if
	}//end for
}//end childProcess

void getDirtyBits(){
	int i;

	for(i = 0; i < 256; i++){
		frame_table.dirty_bit[i] = 0;
	}//end for
}//end dirty 

void checkMessageQueue(){
	
	int pidp;

	msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
	
	char *p;
	pid = strtol(message.mesg_text, &p, 0);

	strcpy(message.mesg_text, "0");
}//end checkMessageQueue

void memMang(){
	
	int i;

	for(i = 0; i < 18; i++){
		if(main_pid_holder[i] == pid){
			if(shared_shm_ptr->proc_read_write[i] == 0){
				my_clock.nano_secs += 10;
				wLog(1, i, 0);
			}else if(shared_shm_ptr->proc_read_write[i] == 1){
				my_clock.nano_secs += 50;
				wLog(2, i, 0);
			}//end else if

			checkTable(i);
			break;
		}//end if
	}//end for

	for(i = 0; i < 18; i++){
		if(main_pid_holder[i] == pid){
			main_pid_holder[i] == 0;
			break;
		}//end if
	}//end for
}//end memmang

void checkTermination(){
	
	int i;
	int s;

	for(i = 0; i < 18; i++){
		if(shared_shm_ptr->proc_call_count[i] >= 10){
			main_pid_holder[i] = 1;
		}//end if
	}//end for

	for(i = 0; i < 18; i++){
		if(main_pid_holder[i] == 1){
			s++;
		}//end if
	}//end for

	if(s >= 18){
		procs_running = 1;
	}
}//end termination

void wLog(int a, int proc_number, int b){
	num_to_log++;

	if(num_to_log % 30 == 0){
		print();
	}//end if
		
	FILE *fp = fopen("log.txt", "a+");
	
	switch(a){
		case 1:
			fprintf(fp, "MASTER: P%d requesting read of address %d at time 0:%d\n",
			 proc_number, shared_shm_ptr->proc_addr_called[proc_number], my_clock.nano_secs);
			break;
		
		case 2:
			fprintf(fp, "MASTER: P%d requesting write of address %d at time 0:%d\n",
			 proc_number, shared_shm_ptr->proc_addr_called[proc_number], my_clock.nano_secs);
			
			break;

		case 3:
			fprintf(fp, "MASTER: Address %d is not in a frame, pagefault\n",
			shared_shm_ptr->proc_addr_called[proc_number]);
			break;

		case 4:
			fprintf(fp, "MASTER: Address %d wrote to frame, %d\n", 
			shared_shm_ptr->proc_addr_called[proc_number], b); 
			break;

		case 5:
			fprintf(fp, "MASTER: Dirty bit of frame %d set\n", b); 
			break;

		case 6:
			fprintf(fp, "MASTER: Address %d is in the page %d\n", 
			shared_shm_ptr->proc_addr_called[proc_number], b); 
			break;

		case 7:
			fprintf(fp, "Running algorith\n");
			break;
			  
	}//end switch

	fclose(fp);
}//end log

void checkTable(int proc_num){
	
	int page_num;
	page_num = shared_shm_ptr->proc_addr_called[proc_num] / 1000;

	if(page_table[proc_num].pages[page_num] == 0){
		wLog(3, proc_num, 0);

		searchFrameTable(proc_num, page_num);
	}//end if 
		else if (page_table[proc_num].pages[page_num] != 0){
			wLog(6, proc_num, page_num);
		}
}//end checkTable

void searchFrameTable(int proc_num, int page_num){

	int i;

	for(i = 0; i < 256; i++){
		if(frame_table.frames[i] == 0){
			frame_table.frames[i] = page_num;
			page_table[proc_num].pages[page_num] = i;
			wLog(4, proc_num, i);
			
			if(shared_shm_ptr->proc_read_write[proc_num] == 1){
				frame_table.dirty_bit[i] = 1;
				wLog(5, proc_num, page_num);
			}//end if
			else if(frame_table.frames[i] != 0){
				chanceIt();
			}//end else if
		}//end if
	}//end for
}//end searchFrameTable

void chanceIt(){
	
	int num = rand() % 255;

	frame_table.ref_flag[num] = 1;

	frame_table.dirty_bit[num] = 1;

	frame_table.ref_flag[num / 7] = 0;

	frame_table.dirty_bit[num / 7] = 0;
}//end chance

void print(){

	int i;
		
	FILE *fp = fopen("log.txt", "a+");
	
	fprintf(fp, "\n");
	
	for(i = 0; i < 256; i++){
		if(i % 32 == 0){
			fprintf(fp, "\n");
		}
			
		fprintf(fp, "%d", frame_table.dirty_bit[i]);
	}//end for

	fprintf(fp, "\n");

	for(i = 0; i < 256; i++){
		if(i % 32 == 0){
			fprintf(fp, "\n");
		}//end if
		fprintf(fp, "%d ", frame_table.ref_flag[i]);
	}//end for

	fprintf(fp, "\n");

	fclose(fp);
}//end print

void cleanMemory(){

	int i;

	if (msgctl(msgid,IPC_RMID,0) < 0 ){
		perror("msgctl");
	}//end if

	for(i = 0; i < 18; i++){
		if(main_pid_holder[i] != 0){
			signal(SIGQUIT, SIG_IGN);
			kill(main_pid_holder[i], SIGQUIT);
		}//end if
	}//end for

	shmdt(shared_shm_ptr);
	shmctl(shared_shmid, IPC_RMID, NULL);
}//clean memory














