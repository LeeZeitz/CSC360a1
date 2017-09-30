#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#define BG "bg"
#define BG_LIST "bglist"
#define BG_KILL "bgkill"
#define BG_STOP "bgstop"
#define BG_START "bgstart"
#define P_STAT "pstat"

#define BUFFER_SIZE 100

struct node *head = NULL;
struct node *curr;
struct node *prev;

struct node
{
	struct node* next;
	struct node* prev;
	int pid;
	char *filename[BUFFER_SIZE];
};


void insert(int data, char* filename)
{
	struct node *newNode = (struct node*) malloc(sizeof(struct node));
	newNode->pid = data;
	*newNode->filename = strdup(filename);
	newNode->next = head;
	newNode->prev = NULL;
	head = newNode;
	return;
}


bool contains(int pid)
{
	bool doesContain = false;
	curr = head;

	while (curr != NULL && doesContain == false)
	{
		doesContain = (curr->pid == pid);
		curr = curr->next;		
	}
	return doesContain;
}


void my_remove(int data)
{
	if (head->pid == data)
	{
		head = head->next;
		return;
	}
	curr = head->next;		

	while (curr != NULL)
	{
		if (curr->pid == data)
		{
			prev->next = curr->next;
			free(curr);
			return;
		}
		prev = curr;
		curr = curr->next;
	}
	return;
}



int main (int argc, char* argv[])
{
	if (argc > 1)
	{
		puts("Too many arguments");
		return 0;	
	}

	else
	{
		while (1)
		{	
			int state;
			int status;
			// Check for terminated/zombie processes
			
			char cmd[BUFFER_SIZE];
			char *token;
			char *args[10];
			char *command;		
	
			printf("\nPMan: >");
			fflush(stdout);
			fgets(cmd, BUFFER_SIZE, stdin);
			
			while ((state = waitpid(-1, &status, WNOHANG)) != 0)
			{
				if(state == -1)
				{
					//perror("Error");
					break;
				}
				else if (state > 0)
				{
					printf("%d exited with status %d\n", state, WEXITSTATUS(status));
					my_remove(state);
				}
			}

			if (!strcmp(cmd, "\n"))
			{
				puts("no commands provided");
				continue;
			}			
	
			token = strtok(cmd, " ");
			
			int i = 0;
			while (token != NULL)
			{	
				args[i] = token;
				token = strtok(NULL, " ");
				i++;
			}
			args[i] = NULL;
			args[i - 1] = strtok(args[i - 1], "\n");
			token = args[0];

			if (!strcmp(token, BG))
			{
				run_process(args[1], args + 1);
			}
			else if (!strcmp(token, BG_LIST))
			{
				list_processes();
			}
			else if (!strcmp(token, BG_KILL))
			{
				ctrl_process(args[1], SIGTERM);
				printf("SIGTERM signal sent to process %s\n", args[1]);
			}
			else if (!strcmp(token, BG_STOP))
			{
				ctrl_process(args[1], SIGSTOP);
			}
			else if (!strcmp(token, BG_START))
			{
				ctrl_process(args[1], SIGCONT);
			}	
			else if (!strcmp(token, P_STAT))
			{
				process_stats(args[1]);
			}
			else
			{	
				printf("%s: command not found\n", token);
			}
			

		}
	}
	return 1;
}


int run_process(char *filename, char* argv[]){
	
	if (filename == NULL)
	{
		puts("no process specified.");
	}
	else
	{
		int pid = fork();
		if (pid < 0)
		{
			fprintf(stderr, "error forking new process");
		}
		// Child process
		else if (pid == 0)
		{
			if (execvp(filename, argv) < 1)
			{
				puts("error when executing file");
			}
		}
		else
		{
			insert(pid, filename);
		}
	}
	return 1;
} 


int ctrl_process(char* pid_s, int signal)
{
	if (pid_s == NULL)
	{
		puts("no process ID specified");
	}
	else
	{
		int pid = atoi(pid_s);
		kill(pid, signal);
	}
	return 1;
}


int list_processes()
{
	int process_count = 0;
	curr = head;
	while (curr != NULL)
	{
		printf("%d %s\n", curr->pid, *curr->filename);
		curr = curr->next;
		process_count++;
	}
	printf("Total background jobs: %d\n", process_count);
	return 1;
}


int process_stats(char *pid_s)
{	
	if (pid_s == NULL)
	{
		puts("no process ID specified");
	}
	else
	{
		int pid = atoi(pid_s);
		if (contains(pid))
		{	
			char comm[BUFFER_SIZE];
			long int utime, stime, vcs, nvcs;
			int rss;
			char state[BUFFER_SIZE];
			FILE *proc;
			char stat[BUFFER_SIZE];
			char status[BUFFER_SIZE];

			sprintf(stat, "/proc/%d/stat", pid);
			sprintf(status, "/proc/%d/status", pid);
			proc = fopen(stat, "r");
			if (!proc)
			{
				fprintf(stderr, "Error: could not open stat file for process %d\n", pid);
				return 1;
			}
			fscanf(proc, "%*d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %*u %d", comm, &utime, &stime, &rss);
			fclose(proc);

			proc = fopen(status, "r");
			if (!proc)
			{
				fprintf(stderr, "Error: could not open status file for process %d\n", pid);
			}
			
			char buffer[BUFFER_SIZE];
			char *token;
			while(1)
			{
				fgets(buffer, BUFFER_SIZE, proc);
				token = strtok(buffer, ":");
				if (!strcmp(token, "State"))
				{
					token = strtok(NULL, "\n");
					sprintf(state, "%s", token);
				}
				else if (!strcmp(token, "voluntary_ctxt_switches"))
				{
					vcs = atoi(strtok(NULL, "\n"));
				}
				else if (!strcmp(token, "nonvoluntary_ctxt_switches"))
				{
					nvcs = atoi(strtok(NULL, "\n"));
					break;
				}
			}


			stime /= (float) sysconf(_SC_CLK_TCK);
			utime /= (float) sysconf(_SC_CLK_TCK); 
			printf("comm: %s\nstate: %s\nutime: %lu\nstime: %lu\nrss: %d\nvoluntary_ctxt_switches: %lu\nnonvoluntary_ctxt_switches: %lu\n", comm, state, utime, stime, rss, vcs, nvcs);
		}
		else
		{
			printf("Error: process %d does not exist\n", pid);
		}
	}
	return 1;
}




