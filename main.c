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
#define cNor	"\x1B[0m"
#define cRed  	"\x1B[31m"
#define cGre	"\x1B[32m"
#define cYel	"\x1B[33m"
#define cBlu	"\x1B[34m"
#define cMag	"\x1B[35m"
#define cCya	"\x1B[36m"
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

static char* tokens[20];
static char* first[50];
static char* second[50];
static int tokenCount;
static int firstSize;
static int secondSize;

static char inputString[100] = "";
static pid_t mainPID;

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
		//printf("inserted job #%d\n", temp->id);
		return temp;
	}
	else
	{
		// Insert at the end of the existing list
		quashJob* cur = jobList;
		while (cur->next)
			cur = cur->next;
		
		temp->id = cur->id + 1;
		//printf("inserted job #%d\n", temp->id);
		cur->next = temp;
		jobCount++;
		return jobList;
	}
}

// Remove a job from the linked list
quashJob* deleteJob(quashJob* targetJob)
{
	if (!jobList) return NULL;
	
	quashJob* curJob;
	quashJob* nextJob;
	
	curJob = jobList;
	nextJob = jobList->next;
	
	if (curJob->pid == targetJob->pid)
	{
		//printf("deleted job #%d\n", curJob->id);
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
			//printf("deleted job #%d\n", nextJob->id);
			jobCount--;
			curJob->next = nextJob->next;
		}
		curJob = nextJob;
		nextJob = nextJob->next;
	}
	return jobList;
}

// Returns the job searched by pid or job id
quashJob* getJob(int target, int searchType)
{
	quashJob* cur = jobList;
	
	switch (searchType) {
	case BY_PID:	
		while (cur)
		{
			if (cur->pid == target)
			{
				//printf("found job %d - %d\n", cur->id, cur->pid);
				return cur;
			}
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

// Put job in the foreground
void putJobFG(quashJob* job) 
{
	//printf("put to fg\n");
	job->status = 1;
	tcsetpgrp(STDIN_FILENO, job->pid);
	
	//printf("middle of fg\n");
	int termStatus;
	while (!waitpid(job->pid, &termStatus, WNOHANG))
	{
		if (job->status == 3)
			return;
	}
	jobList = deleteJob(job);
	
	//printf("end of jobFG\n");
	tcsetpgrp(STDIN_FILENO, quashPID);
}

// Put job in the background
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
	printf(cCya "\n[Quash-2015] %s: " cNor, getcwd(currentDir, 1024));
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
	printf(cCya "Displaying %d tokens:\n" cNor, tokenCount);
	for (int i = 0; i < tokenCount; i++)
		printf("tokens[%d] %s\n", i, tokens[i]);
	printf("\n");
}
void printFirst()
{
	printf(cCya "First %d:\n" cNor, firstSize);
	for (int i = 0; i < firstSize; i++)
		printf("first[%d] %s\n", i, first[i]);
	printf("\n");
}

void printSecond()
{
	printf(cCya "Second %d:\n" cNor, secondSize);
	for (int i = 0; i < secondSize; i++)
		printf("second[%d] %s\n", i, second[i]);
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
	while (firstSize > 0)
	{
		//printf("deleting tokens[%d]\n", tokenCount);
		first[firstSize] = NULL;
		firstSize--;
	}
	while (secondSize > 0)
	{
		//printf("deleting tokens[%d]\n", tokenCount);
		second[secondSize] = NULL;
		secondSize--;
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
		//printf("SIGCHLD's pid: %d\n", pid);
		quashJob* thisJob = getJob(pid, BY_PID);
		
		if (thisJob == NULL)
		{
			//printf("null job\n");
			return;
		}
		
		quashJob* job = getJob(pid, BY_PID);

		jobList = deleteJob(job);
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
		//printf("failure\n");
		exit(EXIT_FAILURE);
		break;
	case 0:
		//printf("child? is %d\n", getpid());
		signal(SIGINT, 	SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
		signal(SIGCHLD, &handleSIGCHLD);
		
		//usleep(20000);
		setpgrp();
		if (mode == 1)
			tcsetpgrp(quashPID, getpid());
		if (mode == 2)
			printf("Running %d-%d in the background\n", ++jobCount, (int)getpid());
		
		// ExecuteCommand start
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
		{
			printf(cRed "Failed to run: %s\n" cNor, *cmd);
			//perror("Quash-2015 ERROR");
		}
		
		exit(EXIT_SUCCESS);
		tcsetpgrp(STDIN_FILENO, quashPID);
		break;
	default:
		usleep(20000);
		//printf("parent? is %d\n", getpid());
		
		setpgid(pid, pid);
		
		jobList = insertJob(pid, *(cmd), file, mode);
		quashJob* thisJob = getJob(pid, BY_PID);
		
		if (mode == 1)
			putJobFG(thisJob);
		
		if (mode == 2)
			putJobBG(thisJob);
		
		//printf("after parent\n");
		
		break;
	}
}

void thePipeLine()
{
	char    line[1000];
    FILE    *fpin, *fpout;
    char arg1[50], arg2[50];
    int i;
printf("ENTERING PIP FOR\n");
	   for(i = 0; first[i]; i++)
	   {
		   strcat(arg1, first[i]);
		   strcat(arg1, " ");
	   }
	   printf("-%s-\n\n", arg1);
	   for(i = 0; second[i]; i++)
	   {
		   strcat(arg2, second[i]);
		   strcat(arg2, " ");
	   }
	   printf("-%s-\n\n", arg2);

    if ((fpin = popen(arg1, "r")) == NULL)
        printf("can't open %s", arg1);

    if ((fpout = popen(arg2, "w")) == NULL)
        printf("popen error");


    while (fgets(line, 1000, fpin) != NULL) {
        if (fputs(line, fpout) == EOF)
            printf("fputs error to pipe");
    }
    if (ferror(fpin))
        printf("fgets error");
    if (pclose(fpout) == -1)
       printf("");
}

void separateCommands(int line)
{
	if (line < 0) return;
	
	for (firstSize; firstSize < line; firstSize++)
		first[firstSize] = tokens[firstSize];
	
	for (int b = line+1; b < tokenCount; b++)
	{
		second[secondSize] = tokens[b];
		secondSize++;
	}
	thePipeLine();
}

int main(int argc, char *argv[], char *envp[])
{
	char input = '\0';
	quashPID = getpgrp();
	signal(SIGINT, 	SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
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
			//printTokens();
			
			int pipeLine;
			pipeLine = -1;
			
			/*for (int i = 0; i < 50; i ++)
			{
				first[i] = '\0';
				second[i] = '\0';
			}*/
			
			for (int i = 0; i < tokenCount; i++)
			{
				if (!strcmp(tokens[i], "|"))
					pipeLine = i;
			}
			if (pipeLine > -1)
			{
				separateCommands(pipeLine);
				//printFirst();
				//printSecond();
			}
			else if (tokenCount == 0)
			{
				// do nothing to prevent seg. fault
			}
			else if (!strcmp(tokens[0], "quit") || !strcmp(tokens[0], "exit") || !strcmp(tokens[0], "q"))
			{
				printf(cRed "Exiting Quash-2015");
				printf(cNor "\n\n");
				exit(EXIT_SUCCESS);
			}
			else if (!strcmp(tokens[0], "set"))
			{
				char theENV[4];
				strncpy(theENV, tokens[1], sizeof(theENV));
				if (strcmp(theENV, "HOME") && strcmp(theENV, "PATH"))
					printf("Invalid  Environment Variable\n");
				else
					setenv(theENV, tokens[1] + 5, 1337);
			}
			else if (!strcmp(tokens[0], "cd"))
			{
				if (tokenCount == 1)
				{
					if (chdir(getenv("HOME")) == -1)
						printf("HOME: Invalid Path\n");
				}
				else if (chdir(tokens[1]) == -1)
					printf("Invalid Path\n");
			}
			else if (!strcmp(tokens[0], "bg") || !strcmp(tokens[tokenCount-1], "&"))
			{
				if (tokens[1] == NULL)
				{
					cleanupInput();
					showPrompt();
					break;
				}
				
				int offset = 0;
				
				// Did this execute from a trailing ampersand?
				if (!strcmp(tokens[tokenCount-1], "&"))
				{
					// No "bg" as first token, so look at -1 index
					offset = -1;
					tokens[tokenCount-1] = NULL;
					tokenCount--;
				}
				
				if (!strcmp("in", tokens[1 + offset]))
					beginJob(tokens + 3 + offset, *(tokens + 2 + offset), 1, 2);
				else if (!strcmp("out", tokens[1 + offset]))
					beginJob(tokens + 3 + offset, *(tokens + 2 + offset), 2, 2);
				else
					beginJob(tokens + 1 + offset, (char *)"STANDARD", 0, 2);
			}
			else if (!strcmp(tokens[0], "fg"))
			{
				//printf("start of fg\n");
				if (tokens[1] == NULL)
				{
					cleanupInput();
					showPrompt();
					break;
				}
				
				int jobID = (int) atoi(tokens[1]);
				quashJob* job = getJob(jobID, BY_ID);
				//printf("midway\n");
				if (job == NULL)
					break;
				if (job->status > 2)
					putJobFG(job);
				
				//printf("end of fg\n");
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
				//printf("Generic execution\n");
				beginJob(tokens, (char *)"STANDARD", 0, 1);
				//printf(cRed "IDK what to do with: %s\n" cNor, tokens[0]);
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