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

// Global variables for linked list
struct node *head = NULL;
struct node *curr;
struct node *prev;

// Node for linked list
struct node
{
	struct node* next;
	struct node* prev;
	int pid;
	char *filename[BUFFER_SIZE];
};


// Inserts a new node into the linked list
// Parameters:
//		# data (int)		: pid of the process being added to the list
//		# filename (char*)	: name of the file/process being added to the list
//
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


// Checks if linked list contains provided pid
// Parameters:
//		# pid (int)		: process id of process to check for in list
//
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


// Removes node with provided pid from linked list
// Parameters:
//		# data (int)		: process id of node to remove from linked list
//
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
		exit(1);	
	}
	else
	{
		while (1)
		{	
			int state, status;
			char cmd[BUFFER_SIZE], *token, *args[10], *command;		
	
			printf("\nPMan: >");
			fflush(stdout);

			// Read in command
			fgets(cmd, BUFFER_SIZE, stdin);
			
			// Check for zombie processes and reap them until none found
			while ((state = waitpid(-1, &status, WNOHANG)) != 0)
			{
				if(state == -1)
				{
					break;
				}
				// if zombie process found, remove it from linked list
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
	
			// parse input command...
			token = strtok(cmd, " ");
			
			int i = 0;
			// parse arguments into array
			while (token != NULL)
			{	
				args[i] = token;
				token = strtok(NULL, " ");
				i++;
			}
			args[i] = NULL;
			args[i - 1] = strtok(args[i - 1], "\n");
			token = args[0];

			// decide which command was entered and execute it
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
				ctrl_process(args[1], SIGCONT);
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


// Creates new process and runs provided file on it
// Parameters:
//		# filename (char*)	: path to file to run on new process
//		# argv[] (char*)	: list of arguments to pass to file to run
//
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
				raise(SIGTERM);
			}
		}
		else
		{
			insert(pid, filename);
		}
	}
	return 1;
} 


// Sends a provided signal to a process
// Parameters: 
//		# pid_s (char*)		: string containing process id of provess to signal
//		# signal (int) 		: the signal to send to the specified process
//
int ctrl_process(char* pid_s, int signal)
{
	if (pid_s == NULL)
	{
		puts("no process ID specified");
	}
	else
	{
		int pid = atoi(pid_s);
		// send signal to process pid
		kill(pid, signal);
	}
	return 1;
}


// Lists all current child processes (all processes in the linked list)
//
int list_processes()
{
	int process_count = 0;
	curr = head;
	// iterate through linked list and print out each node/process
	while (curr != NULL)
	{
		printf("%d: %s\n", curr->pid, *curr->filename);
		curr = curr->next;
		process_count++;
	}
	if (process_count == 0)
	{
		puts("No processes running");
	}
	else
	{
		printf("Total background jobs: %d\n", process_count);
	}
	return 1;
}


// shows statistics for given process
// Parameters:
// 		# pid_s (char *)	: string containing process id to show stats for
//
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
			char comm[BUFFER_SIZE], stat[BUFFER_SIZE], status[BUFFER_SIZE], state[BUFFER_SIZE];
			long int utime, stime, vcs, nvcs;
			int rss;
			FILE *proc;

			// create strings containing file paths to stat and status proc files
			sprintf(stat, "/proc/%d/stat", pid);
			sprintf(status, "/proc/%d/status", pid);

			proc = fopen(stat, "r");
			if (!proc)
			{
				fprintf(stderr, "Error: could not open stat file for process %d\n", pid);
				return 1;
			}

			// read comm, utime, stime, and rss from stat proc file into their variables
			fscanf(proc, "%*d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %*u %d", comm, &utime, &stime, &rss);
			fclose(proc);

			proc = fopen(status, "r");
			if (!proc)
			{
				fprintf(stderr, "Error: could not open status file for process %d\n", pid);
			}
			
			char buffer[BUFFER_SIZE];
			char *token;
			
			// read from status proc file until required variables are found
			while(1)
			{
				// read in next line from status file
				fgets(buffer, BUFFER_SIZE, proc);

				// parse and evaluate read line, and save if it contains a required variable
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

			// print out stats
			printf("comm: %s\nstate: %s\nutime: %lu\nstime: %lu\nrss: %d\nvoluntary_ctxt_switches: %lu\nnonvoluntary_ctxt_switches: %lu\n", comm, state, utime, stime, rss, vcs, nvcs);
		}
		else
		{
			printf("Error: process %d does not exist\n", pid);
		}
	}
	return 1;
}





