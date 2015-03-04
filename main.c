#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#define cNormal	"\x1B[0m"
#define cRed  	"\x1B[31m"
#define cGre	"\x1B[32m"
#define cYel	"\x1B[33m"
#define cBlue	"\x1B[34m"
#define cMag	"\x1B[35m"
#define cCyan	"\x1B[36m"
#define cWhi	"\x1B[37m"
#define BY_PID 	1
#define BY_ID 	2

typedef struct job // struct that defines a job
{ 	
	int id;
	char *name;
	pid_t pid;
	//pid_t pgid; // not yet useful
	int status; 	//1 fg, 2 bg, 3 sus, 4 waiting
	char *descriptor;
	struct job *next;
} quashJob;

static quashJob* jobList = NULL;
static int jobCount = 0;
static pid_t quashPID;
static char* currentDir;
static char* tokens[10];
static int tokenCount;
static char inputString[100] = "";
static pid_t mainPID;
static char path[100] = "";
static char home[100] = "";

quashJob* insertJob(pid_t inPID, /*pid_t inPGID,*/ char* inName, char* inDescriptor, int inStatus)
{
	usleep(420);
	quashJob* temp = (quashJob*)malloc(sizeof(quashJob));
	
	temp->name = (char*) malloc(sizeof(inName));
	temp->name = strcpy(temp->name, inName);
	temp->pid = inPID;
	//temp->pgid = inPGID;
	temp->descriptor = (char*) malloc(sizeof(inDescriptor));
	temp->descriptor = strcpy(temp->descriptor, inDescriptor);
	temp->status = inStatus;
	temp->next = NULL;
	
	if (!jobList)
	{
		// Empty list, so set as head
		jobCount++;
		temp->id = jobCount;
		return temp;
	}
	else
	{
		// Insert at the end of the existing list
		quashJob* cur = jobList;
		while (cur->next)
			cur = cur->next;
		
		temp->id = cur->id + 1;
		cur->next = temp;
		jobCount++;
		return jobList;
	}
}

quashJob* deleteJob(quashJob* targetJob)
{
	if (!jobList) return NULL;
	
	quashJob* curJob;
	quashJob* nextJob;
	
	curJob = jobList;
	nextJob = jobList->next;
	
	if (curJob->pid == targetJob->pid)
	{
		// The head is to be deleted
		curJob = curJob->next;
		jobCount--;
		return curJob;
	}
	
	while (nextJob)
	{
		if (nextJob->pid == targetJob->pid)
		{
			// nextJob is to be deleted, 
			// so use its ->next to connect the new list
			jobCount--;
			curJob->next = nextJob->next;
		}
		curJob = nextJob;
		nextJob = nextJob->next;
	}
	return jobList;
}

quashJob* getJob(int target, int searchType)
{
	quashJob* cur = jobList;
	
	switch (searchType) {
	case BY_PID:	
		while (cur)
		{
			if (cur->pid == target)
				return cur;
			else
				cur = cur->next;
		}
		break;
	case BY_ID:
		while (cur)
		{
			if (cur->id == target)
				return cur;
			else
				cur = cur->next;
		}
		break;
	}
	return NULL;
}

void putJobFG(quashJob* job) 
{
	job->status = 1;
	tcsetpgrp(STDIN_FILENO, job->pid);
	
	int termStatus;
	while (!waitpid(job->pid, &termStatus, WNOHANG))
	{
		if (job->status == 3)
			return;
	}
	jobList = deleteJob(job);
	
	printf("end of jobFG\n");
	tcsetpgrp(STDIN_FILENO, quashPID);
}

void putJobBG(quashJob* job)
{
	if (job == NULL) return;
	
	tcsetpgrp(STDIN_FILENO, quashPID);
}

// Print the list of jobs
void printJobs() 
{
	if (!jobList)
	{
		printf("Empty!\n");
		return;
	}
	
	printf("Current jobs:\n");
	printf("%-5s %-20s %-8s %-12s %-12s \n",
	       "Job", "Name", "PID", "Descriptor", "Status");
	quashJob* cur = jobList;
	
	char status[20] = "";
	
	while (cur)
	{
		if (cur->status == 1)
			strcpy(status, "Foreground");
		else
			strcpy(status, "Background");
		
		printf("%-5d %-20s %-8d %-12s %-12s \n",
		       cur->id, cur->name, cur->pid, cur->descriptor, status);
		cur = cur->next;
	}
	printf("\n");
}

//Display the current user? and directory
void showPrompt() 
{
	currentDir = (char*) calloc(1024, sizeof(char));
	printf("\n[Quash-2015] %s: ", getcwd(currentDir, 1024));
}

// Split up the messy string into tokens, and insert them into tokens[]
void tokenizeString(char inputString[100]) 
{
	char* currentToken = strtok(inputString, " ");
	while (currentToken)
	{
		tokens[tokenCount] = currentToken;
		tokenCount++;
		currentToken = strtok(NULL, " ");
	}
}

// Print tokens[] for debugging purposes
void printTokens() 
{
	printf(cCyan "Displaying %d tokens:\n" cNormal, tokenCount);
	for (int i = 0; i < tokenCount; i++)
		printf("tokens[%d] %s\n", i, tokens[i]);
	printf("\n");
}

// Reset the token count and empty the input string
void cleanupInput() 
{
	while (tokenCount > 0)
	{
		//printf("deleting tokens[%d]\n", tokenCount);
		tokens[tokenCount] = NULL;
		tokenCount--;
	}
	//printf("%d tokens leftover\n", tokenCount);
	bzero(inputString, sizeof(inputString));
}

void handleSIGCHLD(int p)
{
	pid_t pid;
	int termination;
	pid = waitpid(WAIT_ANY, &termination, WUNTRACED | WNOHANG);
	if (pid > 0)
	{
		
	}
	tcsetpgrp(STDIN_FILENO, quashPID);
}

// modes:	1	foreground
//		2	background
void beginJob(char *cmd[], char *file, int desc, int mode)
{
	//printf("inside beginJob\n");

	pid_t pid;
	pid = fork();
	switch (pid) {
	case -1:
		printf("failure\n");
		exit(EXIT_FAILURE);
		break;
	case 0:
		//printf("child? is %d\n", getpid());
		signal(SIGCHLD, &handleSIGCHLD);
		
		usleep(20000);
		setpgrp();
		if (mode == 1)
			tcsetpgrp(quashPID, getpid());
		if (mode == 2)
			printf("[%d] %d\n", ++jobCount, (int)getpid());
		
		
		// ExecuteCommand start
		printf("execvp starting\n");
		
		int commandDesc;
		
		if(desc == STDIN_FILENO)
		{
			commandDesc = open(file, O_RDONLY, 0600);
			dup2(commandDesc, STDIN_FILENO);
			close(commandDesc);
		}
		if(desc == STDOUT_FILENO)
		{
			commandDesc = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
			dup2(commandDesc, STDOUT_FILENO);
			close(commandDesc);
		}
		if (execvp(*cmd, cmd) == -1)
			perror("Quash2015 ERROR");
		
		// ExecuteCommmand end
		
		printf("before exit\n");
		exit(EXIT_SUCCESS);
		printf("after  exit\n");
		tcsetpgrp(STDIN_FILENO, quashPID);
		break;
	default:
		usleep(42042.0);
		//printf("parent? is %d\n", getpid());
		
		setpgid(pid, pid);
		
		jobList = insertJob(pid, *(cmd), file, mode);
		quashJob* thisJob = getJob(pid, BY_PID);
		
		if (mode == 1)
			putJobFG(thisJob);
		/*
		if (mode == 2)
			putJobBG(thisJob);
		*/
		printf("after parent\n");
		
		break;
	}	
}

int main(int argc, char *argv[], char *envp[])
{
	char input = '\0';
	
	quashPID = getpgrp();
	signal(SIGCHLD, &handleSIGCHLD);
	
	showPrompt();
	while(input != EOF)
	{
		input = getchar();
		switch(input)
		{
		case '\n':
			//split up tmp into tokens
			tokenizeString(inputString);
			printTokens();

			if (!strcmp(tokens[0], "quit") || !strcmp(tokens[0], "exit") || !strcmp(tokens[0], "q"))
			{
				printf("Exiting Quash-2015\n\n");
				exit(EXIT_SUCCESS);
			}
			else if (!strcmp(tokens[0], "cd"))
			{
				if (tokenCount == 1)
					chdir(getenv("HOME"));
				else if (chdir(tokens[1]) == -1)
					printf("Invalid Path\n");
			}
			else if (!strcmp(tokens[0], "bg"))
			{
				if (tokens[1] == NULL) break;
				
				if (!strcmp("in", tokens[1]))
					beginJob(tokens + 3, *(tokens + 2), 1, 2);
				else if (!strcmp("out", tokens[1]))
					beginJob(tokens + 3, *(tokens + 2), 2, 2);
				else
					beginJob(tokens + 1, (char *)"STANDARD", 0, 2);
			}
			else if (!strcmp(tokens[0], "fg"))
			{
				printf("start of fg\n");
				if (tokens[1] == NULL) break;
				
				int jobID = (int) atoi(tokens[1]);
				quashJob* job = getJob(jobID, BY_ID);
				printf("midway\n");				
				if (job == NULL)
					break;
				if (job->status > 2)
					putJobFG(job);
				
				printf("end of fg\n");
			}
			else if (!strcmp(tokens[0], "jobs"))
			{
				printJobs();
			}
			/*else if (tokenCount > 3)
			{
				if (strcmp(tokens[tokenCount - 2], "|") == 0 )
				{
					
				}
			}*/
			else
			{
				printf("Generic execution\n");
				beginJob(tokens, (char *)"STANDARD", 0, 1);
				//printf(cRed "IDK what to do with: %s\n" cNormal, tokens[0]);
			}
			
			cleanupInput();
			showPrompt();
			break;
			
		default: 
			strncat(inputString, &input, 1);
			break;
		}
	}
	
	printf("\n");
	return 0;
}
