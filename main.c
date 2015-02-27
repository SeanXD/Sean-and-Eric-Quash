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

int main(int argc, char *argv[], char *envp[])
{
	pid_t pid1;
	
	char tmp[10] = "";
	char c = '\0';
	
	printf("\n[Quash 2015] ");
	while(c != EOF) {
		c = getchar();
		switch(c) {
			case '\n':
				if (!strcmp(tmp,"ls"))
				{
					pid1 = fork();
					
					if (pid1 == 0)
						execve("/bin/ls", argv, envp);
					else
						wait(NULL);
				}
				else if (!strcmp(tmp,"quit") || !strcmp(tmp,"exit"))
				{
					printf("Exit read correctly\n");
				}
				else
				{
					printf("IDK what to do with: %s\n", tmp);
				}
				
				bzero(tmp, sizeof(tmp));
				printf("[Quash 2015] ");
				break;
				
			default: strncat(tmp, &c, 1);
				break;
		}
	}
	
	printf("\n");
	return 0;
}