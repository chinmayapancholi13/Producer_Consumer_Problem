#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/sem.h> 


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

void up(int semid,int subsemid)
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

void down(int semid,int subsemid)
{
	struct sembuf sop;
	sop.sem_num = subsemid;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	if(semop(semid,&sop,1) < 0) {
		perror("semop");
	}
}

int proc_id;
int dead_mat[2][10];
int rfile_mutex;
int num_deletions = 0;

void update_matrix(int i1, int j1, int num)
{
	//printf("Update Matrix function called in consumer %d for : i1 -> %d , j1 -> %d , num -> %d\n",proc_id, i1,j1,num);

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
	
	FILE *fp1 = fopen("matrix.txt", "w");
	for(i=0; i<2; i++)
	{
		for(j=0; j<10; j++)
			printf("%d ", dead_mat[i][j]);
		printf("\n");
	}
	for(i=0; i<2; i++)
	{
		for(j=0; j<10; j++)
		{
			fprintf(fp1, "%d ", dead_mat[i][j]);
		}
		fprintf(fp1, "\n");
	}
	fprintf(fp1,"\0");

	//printf("Consumer %d done updating\n",proc_id);
	fclose(fp1);
}

void sig_handler()
{
	down(rfile_mutex, 0);
	FILE *f1 = fopen("results.txt", "a");
	char c = 'C';
	fprintf(f1, "%c%d : %d\n", c, proc_id, num_deletions);
	fclose(f1);
	up(rfile_mutex, 0);
	//char g = getchar();
	exit(0);
}

int deadlock_case = 1;

int main(int argc, char const *argv[])
{
	int bytes_received1 = 0;

	printf("Consumer %s sending PID to manager\n",argv[1]);

	int msgqid3, msg_flag = IPC_CREAT | 0666;

	msgqid3 = msgget(KEY3, msg_flag);

	start_message s_msg1;
	s_msg1.type = 1006;
	s_msg1.sender_pid = getpid();
	strcpy(s_msg1.msg_txt, argv[1]);

	msgsnd(msgqid3, &s_msg1, strlen(s_msg1.msg_txt)+sizeof(int), 0);	

	//printf("In consumer\n");
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

	int p = 20;

	while(1)
	{
		int q_select = (rand()%2);
		int num_select = (rand()%50 + 1);
		int prob_of_one = (rand()%100 + 1);
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

		if(prob_of_one <= p)
		{
			if(q_select == 0)
			{
				down(fill, 0);

				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 1);
				up(mfile_mutex, 0);
				
				down(mutex, 0);

				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 2);
				up(mfile_mutex, 0);
				
				//if(num_of_messages1 > 0)
				//{

				message msg;
				bzero(msg.msg_txt, BUF_SIZE);
				printf("Consumer %d: Trying to receive from Q0 :\n", proc_id);
				if((bytes_received1 = msgrcv(msgqid1, &msg, BUF_SIZE, 1000, IPC_NOWAIT)) == -1)
				{
					perror("msgrcv here");
					exit(EXIT_FAILURE);
				}
				else
				{
					if(bytes_received1!=0)
					{
						num_deletions++;
					}
					else if(bytes_received1 == 0)
					{
						//printf("Zero bytes received by consumer %d\n",proc_id);
					}
					up(empty, 0);
					//printf("Received Successfully %s\n", msg.msg_txt);
				}

				//}
				
				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 0);
				up(mfile_mutex, 0);
				
				up(mutex, 0);
			}
			else
			{
				down(fill, 1);

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 1);
				up(mfile_mutex, 0);
				
				down(mutex, 1);

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 2);
				up(mfile_mutex, 0);
				
				//if(num_of_messages2 > 0)
				//{

				message msg;
				bzero(msg.msg_txt, BUF_SIZE);
				printf("Consumer %d: Trying to receive from Q1:\n", proc_id);
				if((bytes_received1 = msgrcv(msgqid2, &msg, BUF_SIZE, 1000, IPC_NOWAIT)) == -1)
				{
					perror("msgrcv here");
					exit(EXIT_FAILURE);
				}
				else
				{
					if(bytes_received1!=0)
					{
						num_deletions++;
					}
					else if(bytes_received1 == 0)
					{
						printf("Zero bytes received by consumer %d\n",proc_id);
					}
					up(empty, 1);
					printf("Received Successfully %s\n", msg.msg_txt);
				}

				//}

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 0);
				up(mfile_mutex, 0);

				up(mutex, 1);
			}
		}
		else
		{
			if((deadlock_case == 1 && q_select==0) || (deadlock_case == 0))
			{
				down(fill, 0);
				down(fill, 1);
				
				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 1);
				up(mfile_mutex, 0);
				
				down(mutex, 0);

				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 2);
				up(mfile_mutex, 0);
				
				//if(num_of_messages1 > 0)
				//{

				message msg;
				bzero(msg.msg_txt, BUF_SIZE);
				printf("Consumer %d: Trying to receive from Q0 in 2nd case:\n", proc_id);
				if((bytes_received1= msgrcv(msgqid1, &msg, BUF_SIZE, 1000, IPC_NOWAIT)) == -1)
				{
					perror("msgrcv here");
					exit(EXIT_FAILURE);
				}
				else
				{
					if(bytes_received1!=0) 
					{
						num_deletions++;
					}
					else if(bytes_received1 == 0)
					{
						printf("Zero bytes received by consumer %d\n",proc_id);
					}

					up(empty, 0);
					printf("Received Successfully %s\n", msg.msg_txt);
				}

				//}


				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 1);
				up(mfile_mutex, 0);
				
				down(mutex, 1);

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 2);
				up(mfile_mutex, 0);
				
				//if(num_of_messages2 > 0)
				//{
				//message msg;

				bzero(msg.msg_txt, BUF_SIZE);
				printf("Consumer %d: Trying to receive from Q1 in 2nd case:\n", proc_id);
				if((bytes_received1 = msgrcv(msgqid2, &msg, BUF_SIZE, 1000, IPC_NOWAIT)) == -1)
				{
					perror("msgrcv here");
					exit(EXIT_FAILURE);
				}
				else
				{
					if(bytes_received1!=0)
					{
						num_deletions++;
					}
					else if(bytes_received1 == 0)
					{
						printf("Zero bytes received by consumer %d\n",proc_id);
					}

					up(empty, 1);
					printf("Received Successfully %s\n", msg.msg_txt);
				}

				//}

				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 0);
				up(mfile_mutex, 0);


				up(mutex, 0);				

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 0);
				up(mfile_mutex, 0);
				
				up(mutex, 1);

				/*down(mfile_mutex, 0);
				update_matrix(1, proc_id, 0);
				up(mfile_mutex, 0);*/
				
			}
			else if(deadlock_case == 1 && q_select == 1)
			{
				down(fill, 1);
				down(fill, 0);

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 1);
				up(mfile_mutex, 0);
				
				down(mutex, 1);

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 2);
				up(mfile_mutex, 0);
				
				//if(num_of_messages2 > 0)
				//{

				message msg;
				bzero(msg.msg_txt, BUF_SIZE);
				printf("Consumer %d: Trying to receive from Q1 in 2nd case:\n", proc_id);
				if((bytes_received1 = msgrcv(msgqid2, &msg, BUF_SIZE, 1000, IPC_NOWAIT)) == -1)
				{
					perror("msgrcv here");
					exit(EXIT_FAILURE);
				}
				else
				{
					if(bytes_received1!=0)
					{
						num_deletions++;
					}
					else if(bytes_received1 == 0)
					{
						//printf("Zero bytes received by consumer %d\n",proc_id);
					}

					up(empty, 1);
					printf("Received Successfully %s\n", msg.msg_txt);
				}

				//}

				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 1);
				up(mfile_mutex, 0);
				
				down(mutex, 0);

				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 2);
				up(mfile_mutex, 0);
				
				//if(num_of_messages1 > 0)
				//{
				//message msg;

				bzero(msg.msg_txt, BUF_SIZE);
				printf("Consumer %d: Trying to receive from Q0 in 2nd case:\n", proc_id);
				if((bytes_received1 = msgrcv(msgqid1, &msg, BUF_SIZE, 1000, IPC_NOWAIT)) == -1)
				{
					perror("msgrcv here");
					exit(EXIT_FAILURE);
				}
				else
				{
					if (bytes_received1!=0)
					{
						num_deletions++;
					}
					else if(bytes_received1 == 0)
					{
						//printf("Zero bytes received by consumer %d\n",proc_id);
					}

					up(empty, 0);	
					printf("Received Successfully %s\n", msg.msg_txt);
				}

				//}

				down(mfile_mutex, 0);
				update_matrix(1, proc_id, 0);
				up(mfile_mutex, 0);
				
				up(mutex, 1);

				down(mfile_mutex, 0);
				update_matrix(0, proc_id, 0);
				up(mfile_mutex, 0);
				
				up(mutex, 0);
			}
		}
		//sleep(2);  //random sleep
	}

	return 0;
}