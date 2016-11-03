#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define arraySize 100

char** pipeParse(char userInput[]); //parse userInput string into commands by '|''
char* cdInputSplitter(char *cdInput); //split specific "cd" out from a string
char** commandParse(char command[]); //parse command into path and parameters for execvp() for later use
void chdirc(char **path); //change directory function
int pipeCounter(char str[]); //count the number of pipes

int main(void)
{
	int i=0;
	int j=0;
	int dup2Iterator=0;
	char userInput[arraySize];
	pid_t childProcessPID[arraySize];
	char *fgetsFix;
	char **args; //2D array store 
	char **pipeCommands;
	int status;
	int pipeCount;
	char *cdInput;
		
	while(1)
	{
		printf("guest@linux:~$ ");
		//TESTING: fputs(str, stdout);		
		/*
		control D
		when the user hit control D, the signal itself means the EOF
		it is different from hitting the enter button 
		because hitting the enter button is actually entering '\n'
		into the input buffer of fgets
		
		Ctrl D need to press 2 times to exit??!
		*/	
		if(fgets(userInput, sizeof(userInput), stdin) == NULL)
		{
			printf("^D\n");
			exit(0);
		}

		pipeCount = pipeCounter(userInput);

		int pipefds[2*pipeCount];

		if((fgetsFix = strchr(userInput, '\n')) != NULL) *fgetsFix = '\0'; //replace '\n' with '\0'
		if(strcmp(userInput, "exit") == 0) exit(0); //exit the shell

		pipeCommands = pipeParse(userInput);	

		for(i=0;i<pipeCount;i++)
		{
			if(pipe(pipefds+i*2) < 0)
			{
				perror("pipe create failed");
				exit(0);
			}
		}
		i = j = dup2Iterator = 0;
		while(pipeCommands[j] != NULL)
		{
			args = commandParse(pipeCommands[j]); //commandParse

			// chdir() changing directory
			cdInput = malloc(strlen(userInput));
			strcpy(cdInput, userInput);
			cdInput = cdInputSplitter(cdInput);
			if(strcmp(cdInput, "cd") == 0)
			{
				chdirc(args);
				break;
			}

			// start forking
			childProcessPID[j] = fork();
			if(childProcessPID[j] == -1){
				perror("forking error");
				break;
			}else if(childProcessPID[j] != 0){ //fork off child process
				/*Parent code*/	
				//printf("I'm the parent %d, my child is %d\n", getpid(), childProcessPID);
				//waitpid(-1, &status, 0); //wait for child to exit and join this parent
				//if(wait(childProcessPID) == -1) perror("waiting error");
				//printf("Child complete\n");
			}else{
				/*Child code*/
				//printf("I'm the child %d, my parent is %d\n", getpid(), getppid());
				if( (pipeCommands[j+1] != NULL) && (dup2(pipefds[dup2Iterator+1], STDOUT_FILENO) < 0) )
				{
					perror("dup2 failed");
					break;
				}
				if( (dup2Iterator != 0) && (dup2(pipefds[dup2Iterator-2], STDIN_FILENO) < 0) )
				{
					perror("dup2 failed");
					break;
				}
				for(i=0;i<2*pipeCount;i++) close(pipefds[i]);
				if(execvp(args[0], args) < 0)
				{
					perror("command failed");
					break;
				}
			}
			dup2Iterator+=2;
			j++;
		}
		for(i=0;i<2*pipeCount;i++) close(pipefds[i]);
		for(i=0;i<pipeCount+1;i++) wait(NULL);
	}
}

char** pipeParse(char userInput[])
{
	int i=0;
	int j=0;
	int printindex=0;
	char *token;//token
	char s[] = "|";//token separater
	char **tokenBuffer;

	tokenBuffer = malloc(arraySize*sizeof(char*));
	token = strtok(userInput, s);
	while(token != NULL)
	{
		tokenBuffer[i] = malloc(strlen(token));
		strcpy(tokenBuffer[i], token);
		token = strtok(NULL, s);
		i++;
	}
	//test if the strings in the double pointer is right
	/*
	while(tokenBuffer[printindex] != NULL)
	{
		printf("%d: %s\n", printindex, tokenBuffer[printindex]);
		printindex++;
	}
	*/
	return tokenBuffer;
}

char* cdInputSplitter(char *cdInput)
{
	char s[] = " ";
	char *token;
	token = strtok(cdInput, s);
	return token;
}

char** commandParse(char command[])
{
	int i=0; //iterator for tokenBuffer
	int j=0;
	int printindex=0;//loop iterator
	char **tokenBuffer;//array of char pointers for storing each token
	char s[] = " \t";//token separater
	char *token;//token
	char **args;
	
	tokenBuffer = calloc(arraySize, sizeof(char*));
	token = strtok(command, s);
	while(token != NULL)
	{
		tokenBuffer[i] = malloc(strlen(token));
		strcpy(tokenBuffer[i], token);
		//printf("%s\n", token);
		token = strtok(NULL, s);
		i++;
	}
	return tokenBuffer;
}

void chdirc(char **path)
{
	int i = 0;
	int j = 0;
	int ret;
	char directory[arraySize];

	path+=1;
	memset(&directory[0], 0, sizeof(directory));
	/*
	while(path[j] != NULL)
	{
		printf("printing the content of args");
		printf("%d: %s\n", j, path[i]);
		j++;
	}
	*/
	while(path[i++] != NULL) strcat(directory, path[i]);
	// printf("%s\n", directory);
	if((ret = chdir(directory)) != 0) perror("chdir error");
}

int pipeCounter(char userInput[])
{
	int i = 0;
	int pipeCount = 0;
	while(userInput[i] != '\n')
	{
		if(userInput[i] == '|') pipeCount++;
		i++;
	}
	return pipeCount;
}
