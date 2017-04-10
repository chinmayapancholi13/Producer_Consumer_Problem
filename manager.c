#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
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

#define NUM 5 	//num of prods or consumers

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

int dead_mat[2][10];

int adj[12][12];

void create_mat()		//create adjacenecy matrix of graph
{
	int i, j;
	for(i=0; i<12; i++)
	{
		for(j=0; j<12; j++)
		{
			adj[i][j] = 0;
		}
	}

	for(i=0; i<2; i++)
	{
		for(j=0; j<10; j++)
		{
			if(i==0)
			{
				if(dead_mat[i][j] == 1)
				{
					adj[j][10] = 1;
				}
				else if(dead_mat[i][j] == 2)
				{
					adj[10][j] = 1;
				}
			}
			if(i==1)
			{
				if(dead_mat[i][j] == 1)
				{
					adj[j][11] = 1;
				}
				else if(dead_mat[i][j] == 2)
				{
					adj[11][j] = 1;
				}
			}
		}
	}
	printf("The adjacency matrix created of the graph is : \n");
	for(i=0;i<12;i++)
	{
		for(j=0;j<12;j++)
		{
			printf("%d\t",adj[i][j]);
		}
		printf("\n");
	}
}

int visited[12];		

int detect_cycle(int node)		//detect cycle in directed graph
{
	int i;
	visited[node] = 1;
	for(i=0; i<12; i++)
	{
		if(adj[node][i] == 1)
		{
			if(visited[i] == 1)
			{
				return 1;
			}
			if(visited[i] == 0)
			{
				return detect_cycle(i);
			}
		}
	}
	visited[node] = 2;
	return 0;
}

int parent_array[12];

int print_cycle(int node)
{
	int i;
	visited[node] = 1;
	for(i=0;i<12;i++)
	{
		if(adj[node][i] == 1)
		{
			if(visited[i] == 1)
			{
				int curr_node = parent_array[node];
				printf("Cycle : %d",node);
				while(1)
				{
					if(parent_array[curr_node] == curr_node)
					{
						printf("\n");
						break;
					}
					printf(" <- %d",curr_node);
					curr_node = parent_array[curr_node];
				}
				return 1;
			}
			else if(visited[i] == 0)
			{
				parent_array[i] = node;
				return print_cycle(i);
			}
		}
	}
	visited[node] = 2;
	return 0;
}

void print_adj_graph()
{
	int i,j;
	printf("The adjacency matrix created of the graph is : \n");
	for(i=0;i<12;i++)
	{
		for(j=0;j<12;j++)
		{
			printf("%d\t",adj[i][j]);
		}
		printf("\n");
	}
}

int main(int argc, char const *argv[])
{
	int msgqid3, msg_flag = IPC_CREAT | 0666;

	msgqid3 = msgget(KEY3, msg_flag);

	struct msqid_ds buf;

	if(msgctl(msgqid3, IPC_RMID, &buf)!=0)
	{	
		perror("msgctl_0");
		exit(EXIT_FAILURE);
	}

	msgqid3 = msgget(KEY3, msg_flag);

	if(msgqid3 < 0)
	{
		perror("msgget_0");
		exit(EXIT_FAILURE);		
	}

	FILE *fp_res = fopen("results.txt","w");

	if(fp_res == NULL)
	{
		perror("Error opening file 'results.txt'\n");
		exit(EXIT_FAILURE);
	}

	fclose(fp_res);

	int msgqid1, msgqid2;
	pid_t producer[NUM], consumer[NUM];		//arrays to store the pid of producer and consumer
	int i,j;
	int mutex, empty, fill, mfile_mutex, rfile_mutex;		//the semaphores

	mutex = semget(MUTEX, 2, msg_flag);
	empty = semget(EMPTY, 2, msg_flag);
	fill = semget(FILL, 2, msg_flag);
	mfile_mutex = semget(MFILE, 1, msg_flag);
	rfile_mutex = semget(RFILE, 1, msg_flag);
	
	semctl(mutex, 0, IPC_RMID, 0);
	semctl(mutex, 1, IPC_RMID, 0);
	semctl(empty, 0, IPC_RMID, 0);
	semctl(empty, 1, IPC_RMID, 0);
	semctl(fill, 0, IPC_RMID, 0);
	semctl(fill, 1, IPC_RMID, 0);
	semctl(mfile_mutex, 0, IPC_RMID, 0);
	semctl(rfile_mutex, 0, IPC_RMID, 0);

	mutex = semget(MUTEX, 2, msg_flag);
	empty = semget(EMPTY, 2, msg_flag);
	fill = semget(FILL, 2, msg_flag);
	mfile_mutex = semget(MFILE, 1, msg_flag);
	rfile_mutex = semget(RFILE, 1, msg_flag);
	
	semctl(mutex, 0, SETVAL, 1);	//set subsemaphore as 1
	semctl(mutex, 1, SETVAL, 1);	//set subsemaphore as 1
	semctl(empty, 0, SETVAL, BUF_SIZE);		//set subsemaphore as BUF_SIZE
	semctl(empty, 1, SETVAL, BUF_SIZE);		//set subsemaphore as BUF_SIZE
	semctl(fill, 0, SETVAL, 0);		//set subsemaphore as 0
	semctl(fill, 1, SETVAL, 0);		//set subsemaphore as 0
	semctl(mfile_mutex, 0, SETVAL, 1);	//set subsemaphore as 1
	semctl(rfile_mutex, 0, SETVAL, 1);	//set subsemaphore as 1
		
	for(i=0; i<2; i++)
	{
		for(j=0; j<10; j++)
		{
			dead_mat[i][j] = 0;		//initialize the matrix.txt file as all 0's
		}
	}

	FILE *fp = fopen("matrix.txt", "w");

	if(fp == NULL)
	{
		perror("Error opening file 'matrix.txt'\n");
		exit(EXIT_FAILURE);		
	}
	
	for(i=0; i<2; i++)
	{
		for(j=0; j<10; j++)
		{
			fprintf(fp, "%d ", dead_mat[i][j]);		//write to the matrix.txt file
		}
		fprintf(fp, "\n");
	}

	fprintf(fp,"\0");

	fclose(fp);
	
	msgqid1 = msgget(KEY1, msg_flag);		//create message queue and return the message queue identifier

	if(msgctl(msgqid1, IPC_RMID, &buf) != 0) 	//Immediately remove the message queue, awakening all waiting reader and writer processes 
	{
		perror("msgctl here1");
		exit(EXIT_FAILURE);		
	}

	msgqid1 = msgget(KEY1, msg_flag);		//create message queue and return the message queue identifier

	if(msgqid1 < 0)
	{
		perror("msgget2");
		exit(EXIT_FAILURE);		
	}

	msgqid2 = msgget(KEY2, msg_flag);		//create message queue and return the message queue identifier

	if(msgctl(msgqid2, IPC_RMID, &buf) != 0) 	//Immediately remove the message queue, awakening all waiting reader and writer processes 
	{
		perror("msgctl here3");
		exit(EXIT_FAILURE);		
	}

	msgqid2 = msgget(KEY2, msg_flag);		//create message queue and return the message queue identifier

	if(msgqid2 < 0)
	{
		perror("msgget4");
		exit(EXIT_FAILURE);		
	}

	msgqid3 = msgget(KEY3, msg_flag);

	if(msgctl(msgqid3, IPC_RMID, &buf) != 0)
	{
		perror("msgctl here4");
		exit(EXIT_FAILURE);
	}

	msgqid3 = msgget(KEY3, msg_flag);

	if(msgqid3 < 0)
	{
		perror("msgget3");
		exit(EXIT_FAILURE);
	}

	printf("Creating producers and consumers....\n");

	pid_t curr_pid = getpid();

	for(i=0; i<NUM; i++)		//create 5 producers in separate xterm windows
	{
		pid_t pr;
		if(curr_pid == getpid())
		{
			pr = fork();
		}
		
		if(pr==0)
		{
			char arg[10];
			sprintf(arg, "%d", i);

			char cmd1[1000];

			strcpy(cmd1,"./producer");
			strcat(cmd1," ");
			strcat(cmd1,arg);

			//execlp("xterm", "xterm", "-hold", "-e", "bash", "-c", cmd1, (void*)NULL);
			execlp("xterm", "xterm", "-hold", "-e", "./producer", arg, NULL);
		}
		else if (pr!=0)
		{
			//producer[i] = pr;
		}
	}
	
	for(i=0; i<NUM; i++)		//create 5 consumers in separate xterm windows
	{
		pid_t cr;
		if(curr_pid == getpid())
		{
			cr = fork();
		}

		if(cr==0)
		{
			char arg1[10];
			sprintf(arg1, "%d", i+5);

			char cmd1[1000];

			strcpy(cmd1,"./consumer");
			strcat(cmd1," ");
			strcat(cmd1,arg1);

			//execlp("xterm", "xterm", "-hold", "-e", "bash", "-c", cmd1, (void*)NULL);
			execlp("xterm", "xterm", "-hold", "-e", "./consumer", arg1, NULL);
		}
		else if (cr!=0)
		{
			//consumer[i] = cr;
		}
	}

	start_message s_msg1_frm_proc;

	for(i=0;i<NUM;i++)
	{
		bzero(s_msg1_frm_proc.msg_txt, BUF_SIZE);
		if(msgrcv(msgqid3, &s_msg1_frm_proc, BUF_SIZE, 1005, 0) == (-1))
		{
			perror("msgrcv_0");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("Values received : \n");
			printf("s_msg1_frm_proc.type -> %ld\n",s_msg1_frm_proc.type);
			printf("s_msg1_frm_proc.msg_txt -> %s\n",s_msg1_frm_proc.msg_txt);
			printf("s_msg1_frm_proc.sender_pid -> %d\n",s_msg1_frm_proc.sender_pid);

			producer[(int)atoi(s_msg1_frm_proc.msg_txt)] = (int)s_msg1_frm_proc.sender_pid;
		}
	}

	for(i=0;i<NUM;i++)
	{
		bzero(s_msg1_frm_proc.msg_txt, BUF_SIZE);
		if(msgrcv(msgqid3, &s_msg1_frm_proc, BUF_SIZE, 1006, 0) == (-1))
		{
			perror("msgrcv_0");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("s_msg1_frm_proc.type -> %ld\n",s_msg1_frm_proc.type);
			printf("s_msg1_frm_proc.msg_txt -> %s\n",s_msg1_frm_proc.msg_txt);
			printf("s_msg1_frm_proc.sender_pid -> %d\n",s_msg1_frm_proc.sender_pid);

			consumer[(int)atoi(s_msg1_frm_proc.msg_txt)-5] = (int)s_msg1_frm_proc.sender_pid;
		}
	}

	printf("PIDs received are : \n");
	for(i=0;i<NUM;i++)
	{
		printf("%d\t%d\n",producer[i], consumer[i]);
	}
	printf("\n");

	printf("Manager here : \n");
	
	while(1)	//check for deadlock at intervals of 2 sec
	{
		i=0;
		fp = fopen("matrix.txt", "r");	//check if mutual exclusion needed here
		printf("Scanning the matrix.txt file : \n");
		
		/*
		size_t count = 0; 

	    char *str;
	    str = (char*)malloc(1000);

    	i=0;
	    while(fgets(str, 990, fp)!=NULL)
	    {
	    	str[strlen(str)-2]='\0';
	    	printf("READ LINE -> %s\n",str);
	    	for(j=0; j<10; j++)
	    	{
	    		dead_mat[i][j] = str[2*j]-'0';
	    		printf("dead_mat[%d][%d] --> %d\n",i,j,dead_mat[i][j]);
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
		create_mat();
		for(i=0;i<12;i++)
		{
			visited[i] = 0;
		}
		int ret;
		for(i=0; i<12; i++)
		{
			int f;
			for(f=0; f<12; f++)
				visited[f] = 0;
			ret = detect_cycle(i);
			if(ret == 1);
				break;
		}//detect a cycle
		if(ret == 1)
		{
			printf("Deadlock detected\n\n");
			print_adj_graph();
			int k;
			int f;
			printf("The cycle in the adjacency graph is : \n");
				
			for(f=0; f<12; f++)
			{
				for(k=0;k<12;k++)
				{
					parent_array[k]=k;
				}
				for(i=0;i<12;i++)
				{
					visited[i] = 0;
				}
				int x;
				x = print_cycle(f);		//print cycle in case deadlock is detected and then break
				if(x==1)
					break;
			}
			break;
		}
		else
		{
			sleep(2);		//sleep for 2 sec and again check for deadlock
		}
	}
	
	for(i=0;i<NUM;i++)
	{
		//printf("%d %d\n",producer[i], consumer[i]);
	}

	for(i=0; i<NUM; i++)
	{
		kill(producer[i], SIGRTMIN+1);		//terminate the producers
		kill(consumer[i], SIGRTMIN+1);		//terminate the consumers
	}

	printf("All producers and consumers terminated.\n");
	printf("Enter a character to proceed!!\n");
	
	char g1 = getchar();

	char c1;
	int val1, val2;

	int total_prod_insertions = 0;
	int total_cons_deletions = 0;

	FILE *res_calc_ptr;
	res_calc_ptr = fopen("results.txt","r");
	//printf("here");		//file for storing the results
	for(i=0;i<2*NUM;i++)
	{
		fscanf(res_calc_ptr,"%c%d : %d\n",&c1,&val1,&val2);

		//printf("c1 -> %c | val1 -> %d | val2 -> %d\n",c1, val1, val2);

		if(c1=='P')
		{	
			total_prod_insertions+=(val2);
		}
		else if(c1=='C')
		{
			total_cons_deletions+=(val2);
		}
	}

	fclose(res_calc_ptr);

	FILE * res_update = fopen("results.txt","a");

	fprintf(res_update,"\n");
	fprintf(res_update, "PRODUCERS INSERTIONS : %d\n",total_prod_insertions);
	fprintf(res_update, "CONSUMERS DELETIONS : %d\n",total_cons_deletions);

	fclose(res_update);

	return 0;
}
