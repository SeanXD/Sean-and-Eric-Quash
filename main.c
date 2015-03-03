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

static char* currentDir;
static char* tokens[10];
static int tokenCount;
static char inputString[100] = "";
static pid_t mainPID;

static char path[100] = "";
static char home[100] = "";

#define cNormal		"\x1B[0m"
#define cRed  		"\x1B[31m"
#define cGreen		"\x1B[32m"
#define cYellow		"\x1B[33m"
#define cBlue		"\x1B[34m"
#define cMagenta	"\x1B[35m"
#define cCyan		"\x1B[36m"
#define cWhite		"\x1B[37m"

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
}

// Reset the token count and empty the input string
void cleanupInput()
{
	tokenCount = 0;
	bzero(inputString, sizeof(inputString));
}

void beginJob(char *cmd[], char *file)
{

}

int main(int argc, char *argv[], char *envp[])
{
	char input = '\0';
	
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

				if (!strcmp(tokens[0], "ls") || !strcmp(tokens[0], "dir"))
				{
					mainPID = fork();
					if (mainPID == 0)
					{
						execve("/bin/ls", argv, envp);
					}
					else
						wait(NULL);
				}
				else if (!strcmp(tokens[0], "quit") || !strcmp(tokens[0], "exit") || !strcmp(tokens[0], "q"))
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
					if (tokens[1] == NULL)
						beginJob(tokens, "STANDARD"/*, 0, 'f'*/);
					if (!strcmp("in", tokens[1]))
						beginJob(tokens + 3, *(tokens + 2)/*, 1, 'b'*/);
					else if (!strcmp("out", tokens[1]))
						beginJob(tokens + 3, *(tokens + 2)/*, 2, 'b'*/);
					else
						beginJob(tokens + 1, "STANDARD"/*, 0, 'b'*/);
				}
				else if (!strcmp(tokens[0], "fg"))
				{
					
				}
				else if (!strcmp(tokens[0], "jobs"))
				{
					
				}
				else if (tokenCount == 3)
				{
					if (!strcmp(tokens[1], "|"))
					{
						
					}
				}			
				else if (tokenCount > 3)
				{
					if (strcmp(tokens[tokenCount - 2], "|") == 0 )
					{
						
					}
				}
				else
				{
					beginJob(tokens, "STANDARD"/*, 0, 'f'*/);
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
