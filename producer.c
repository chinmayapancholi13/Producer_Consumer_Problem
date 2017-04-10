#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/sem.h> 
#include <signal.h>


#define KEY1 1000
#define KEY2 2000
#define KEY3 3000

#define BUF_SIZE 100

#define MUTEX 10
#define EMPTY 20
#define FILL 30
#define MFILE 40
#define RFILE 50

#define NUM 5

typedef struct message 		//structure for message queue
{
	long type;
	char msg_txt[BUF_SIZE];
} message;

typedef struct start_message
{
	long type;
	int sender_pid;
	char msg_txt[BUF_SIZE];
} start_message;

int proc_id;

void up(int semid,int subsemid)		//perform up for semaphores
{
	struct sembuf sop;
	sop.sem_num = subsemid;
	sop.sem_op = 1;
	sop.sem_flg = 0;
	if(semop(semid,&sop,1) < 0) 
	{
		perror("semop");
	}
}

void down(int semid,int subsemid)		//perform down for semaphores
{
	struct sembuf sop;
	sop.sem_num = subsemid;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	if(semop(semid,&sop,1) < 0) 
	{
		perror("semop");
	}
}

int down_ipc(int semid,int subsemid)		//perform down with ipc_nowait
{
	struct sembuf sop;
	sop.sem_num = subsemid;
	sop.sem_op = -1;
	sop.sem_flg = IPC_NOWAIT;
	if(semop(semid,&sop,1) < 0) 
	{
		return 0;
	}
	return 1;
}

int dead_mat[2][10];

//function to update the matrix.txt file whenever needed 
void update_matrix(int i1, int j1, int num)
{
	//printf("Update Matrix function called in producer %d for : i1 -> %d , j1 -> %d , num -> %d\n",proc_id, i1,j1,num);

	int i,j;

	FILE *fp;
	fp = fopen("matrix.txt", "r");	//check if mutual exclusion needed here

	/*
	size_t count = 0; 

    char *str;
    str = (char*)malloc(1000);

	i=0;
    while(fgets(str, 990, fp)!=NULL)
    {
    	str[strlen(str)-2]='\0';
    	for(j=0; j<10; j++)
    	{
    		dead_mat[i][j] = str[2*j]-'0';
    	}
    	i++;
    }
    */
    for(i=0;i<2;i++)
    {
    	for(j=0;j<10;j++)
    	{
    		fscanf(fp,"%d",&dead_mat[i][j]);
    	}
    }

	fclose(fp);

	dead_mat[i1][j1] = num;

	for(i=0; i<2; i++)
	{
		for(j=0; j<10; j++)
		{
			printf("%d\t", dead_mat[i][j]);
		}
		printf("\n");
	}
	
	FILE *fp1 = fopen("matrix.txt", "w");
	for(i=0; i<2; i++)
	{
		for(j=0; j<10; j++)
		{
			fprintf(fp1, "%d ", dead_mat[i][j]);
		}
		fprintf(fp1, "\n");
	}
	fprintf(fp1,"\0");

	//printf("Producer %d done updating\n",proc_id);
	fclose(fp1);
}

int num_insertions = 0;
int rfile_mutex;

//handle the signal sent from manager and update results.txt and exit
void sig_handler(int signo)
{
	down(rfile_mutex, 0);
	FILE *f1 = fopen("results.txt", "a");
	char c = 'P';
	fprintf(f1, "%c%d : %d\n", c, proc_id, num_insertions);
	fclose(f1);
	up(rfile_mutex, 0);
	//char g = getchar();
	exit(0);
}

int main(int argc, char const *argv[])
{
	printf("Producer %s sending PID to manager\n",argv[1]);

	int msgqid3, msg_flag = IPC_CREAT | 0666;

	msgqid3 = msgget(KEY3, msg_flag);

	start_message s_msg1;
	s_msg1.type = 1005;
	s_msg1.sender_pid = getpid();
	strcpy(s_msg1.msg_txt, argv[1]);

	msgsnd(msgqid3, &s_msg1, strlen(s_msg1.msg_txt)+sizeof(int), 0);	

	//printf("In producer\n");
	int msgqid1, msgqid2;
	int mutex, empty, fill, mfile_mutex;
	int status_ret;
	msgqid1 = msgget(KEY1, msg_flag);		//create message queue and return the message queue identifier
	msgqid2 = msgget(KEY2, msg_flag);		//create message queue and return the message queue identifier
	struct msqid_ds buf;

	srand(time(NULL));

	signal(SIGRTMIN+1, sig_handler);

	mutex = semget(MUTEX, 2, msg_flag);
	empty = semget(EMPTY, 2, msg_flag);
	fill = semget(FILL, 2, msg_flag);
	mfile_mutex = semget(MFILE, 1, msg_flag);
	rfile_mutex = semget(RFILE, 1, msg_flag);

	proc_id = atoi(argv[1]);

	int i,j;
	int random_sleep_duration;

	while(1)
	{
		int q_select = (rand()%2);		//select a randum queue Q0 or Q1
		int num_select = (rand()%50 + 1);	//select a random number between 1-50 for insertion 
		int num_of_messages1, num_of_messages2;

		if(msgctl(msgqid1,IPC_STAT,&buf) < 0) 
		{
			perror("msgctl");
		}

		num_of_messages1 = buf.msg_qnum;

		if(msgctl(msgqid2,IPC_STAT,&buf) < 0) 
		{
			perror("msgctl");
		}

		num_of_messages2 = buf.msg_qnum;

		if(q_select==0)
		{
			int r = down_ipc(empty, 0);
			if(r==0)			
			{
				//printf("Zero returned by down_ipc\n");
				continue;
			}

			down(mfile_mutex, 0);		
			update_matrix(0, proc_id, 1);		//update matrix.txt
			up(mfile_mutex, 0);
			
			down(mutex, 0);		//for mutual exclusion of critical section

			down(mfile_mutex, 0);
			update_matrix(0, proc_id, 2);
			up(mfile_mutex, 0);

			printf("Producer %d: Trying to insert in Q0...\n", proc_id);	//entered critical section
			if(r==1)
			{
				message msg;
				bzero(msg.msg_txt, BUF_SIZE);
				msg.type = 1000;
				sprintf(msg.msg_txt, "%d", num_select);
				//strcpy(msg.msg_txt, (char*)num_select);
				if(msgsnd(msgqid1, &msg, strlen(msg.msg_txt), 0) == -1)		//insert a message in queue
				{
					perror("1. msgsnd here");
					exit(EXIT_FAILURE);
				}
				else
				{
					num_insertions++;		//on successful insertion, increment insertions count
					printf("Producer %d: Successfully inserted in Q0 value %d...\n", proc_id, num_select);
				}
			
				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 0);
				up(mfile_mutex, 0);

				up(fill, 0);

				up(mutex, 0);
			}
		}
		else if(q_select==1)
		{
			int r = down_ipc(empty, 1);

			if(r==0)
			{
				//printf("Zero returned by down_ipc\n");
				continue;
			}

			down(mfile_mutex, 0);
			update_matrix(1, proc_id, 1);
			up(mfile_mutex, 0);
			
			down(mutex, 1);

			down(mfile_mutex, 0);
			update_matrix(1, proc_id, 2);
			up(mfile_mutex, 0);

			if(r==1)
			{
				printf("Producer %d: Trying to insert in Q1...\n", proc_id);
	
				message msg;
				bzero(msg.msg_txt, BUF_SIZE);
				msg.type = 1000;
				sprintf(msg.msg_txt, "%d", num_select);
				//strcpy(msg.msg_txt, (char*)num_select);
				if(msgsnd(msgqid2, &msg, strlen(msg.msg_txt), 0) == -1)
				{
					perror("1. msgsnd here");
					exit(EXIT_FAILURE);
				}
				else
				{
					num_insertions++;
					printf("Producer %d: Successfully inserted in Q1 value %d...\n", proc_id, num_select);
				}
			
				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 0);
				up(mfile_mutex, 0);

				up(fill, 1);

				up(mutex, 1);				
			}
		}
		random_sleep_duration = (rand()%10+1);
		random_sleep_duration*=(10000);
		usleep(random_sleep_duration);

		//sleep(2);  //random sleep
	}

	return 0;
}