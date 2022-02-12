/* Xiaoying Li
 * lixiaoyi@oregonstate.edu
 * Assignment 2: smallsh */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>


// Global variables for foreground and background processes flags.
bool foregroundOnly = false;
bool background = false;


/*
 * Signal handler for SIGTSTP. When a SIGTSTP signal is received, this function switches states between
 * normal condition and foreground only condition.
 */
void handle_SIGTSTP(int signo)
{
	// If the current state is the normal condition, switch to the foreground only condition, where
	// subsequent commands can no longer be run in the background. In this state, the & operator should 
	// simply be ignored, run all such commands as if they were foreground processes.
	if (foregroundOnly == false) {
		foregroundOnly = true;
		// Display an informative message.
		char* message = "\nEntering foreground-only mode (& is now ignored)\n: ";
		write(STDOUT_FILENO, message, 52);
		fflush(stdout);
	}

	// If the current state is the foreground only condition, switch to the normal condition, where the &
	// operator is once again honored for subsequent commands, allowing them to be placed in the background.
	else {
		foregroundOnly = false;
		// Display an informative message.
		char* message = "\nExiting foreground-only mode\n: ";
		write(STDOUT_FILENO, message, 32);
		fflush(stdout);
	}
}


int main()
{
	char* inputCommand = malloc(2048);	// Command lines support maximum length of 2048 characters.
	// Array to store commond and its arguments, command lines support a maximum of 512 arguments.
	char* arguments[513];
	char expansion[2048];	// Prepare for variable expansion of "$$" in a command.
	
	// Prepare for the redirection of standard input and standard output.
	char* inputFile = NULL;
	char* outputFile = NULL;
	int targetFile = -1;

	// Prepare for storing the pids of non-completed background processes.
	pid_t backPids[512];
	int pidsNumber = 0;
	pid_t pid;

	int status = 0;

	// Set up action and handler for ^C
	struct sigaction sigintAction = { 0 };	// Initialize sigintAction struct to be empty.
	// Fill out the sigintAction struct.
	sigintAction.sa_handler = SIG_IGN;	 // Register SIG_INT as the signal handler.
	sigfillset(&sigintAction.sa_mask);	// Block all catchable signals while SIG_INT is running.
	sigintAction.sa_flags = 0;	// No flags set.
	sigaction(SIGINT, &sigintAction, NULL);	// Install the signal handler.

	// Set up action and handler for ^Z
	struct sigaction sigtstpAction = { 0 };	// Initialize sigtstpAction struct to be empty.
	// Fill out the sigtstpAction struct.
	sigtstpAction.sa_handler = &handle_SIGTSTP;	// Register handle_SIGTSTP as the signal handler.
	sigfillset(&sigtstpAction.sa_mask);	// Block all catchable signals while handle_SIGTSTP is running.
	sigtstpAction.sa_flags = SA_RESTART;	// No flags set.
	sigaction(SIGTSTP, &sigtstpAction, NULL);	// Install the signal handler.

	while (1) {
		background = false;	// Set background flag to false.

		printf(": ");	// Use the colon symbol as a prompt for each command line.
		fflush(stdout);	// Flush out the output buffers each time print.

		// Get user input.
		size_t length = 0;
		ssize_t nread;
		nread = getline(&inputCommand, &length, stdin);
		if (nread == -1) {
			clearerr(stdin);
		}

		int argumentsNumber = 0;	// Counter of arguments plus the command word.
		// Tokenize the input commond, and the delimiter is space and line break.
		char* token = strtok(inputCommand, " \n");

		while (token != NULL) {
			// If the "<" word appeared, standard input is to be redirected.
			if (strcmp(token, "<") == 0) {
				// Continue tokenizing the input commond, and the delimiter is still space and line break.
				token = strtok(NULL, " \n");
				// The following word is the filename of the input file.
				inputFile = strdup(token);
				token = strtok(NULL, " \n");
			}

			// If the ">" word appeared, standard output is to be redirected.
			else if (strcmp(token, ">") == 0) {
				// Continue tokenizing the input commond, and the delimiter is still space and line break.
				token = strtok(NULL, " \n");
				// The following word is the filename of the output file.
				outputFile = strdup(token);
				token = strtok(NULL, " \n");
			}

			else {
				// Expand any instance of "$$" in a command into the process ID of the shell itself.
				if (strstr(token, "$$") != NULL) {
					// Get the shell pid and convert it to a string.
					int shellPid = getpid();
					char pidString[256];
					sprintf(pidString, "%d", shellPid);

					// Set up necessary variables to manipulate the token,
					// to avoid changing the original one by accident.
					strcpy(expansion, token);
					char* copy = expansion;
					char temp[2048];

					// Search for strings include "$$".
					while (copy = strstr(copy, "$$")) {
						// If found, copy the string up to the point before "$$".
						strncpy(temp, expansion, copy - expansion);
						temp[copy - expansion] = '\0';
						// Append the string of shell pid to the end of the sub-string.
						strcat(temp, pidString);
						strcat(temp, copy + strlen("$$"));
						strcpy(expansion, temp);
						copy++;
					}

					// Copy the expanded string to the original token.
					token = expansion;
				}

				// Add the token to the arguments array, and update the number of arguments.
				arguments[argumentsNumber] = strdup(token);
				token = strtok(NULL, " \n");
				argumentsNumber++;
			}
		}

		// If the last word of the commond is "&", the command is to be executed in the background.
		if (arguments[argumentsNumber - 1] != NULL && strcmp(arguments[argumentsNumber - 1], "&") == 0) {
			arguments[argumentsNumber - 1] = '\0';
			if (foregroundOnly == false) {
				background = true;
			}
		}

		arguments[argumentsNumber] = NULL;

		// A blank line (one without any commands) or a command line should do nothing.
		// Re-prompt for another command when a blank line or a command line is received.
		if (arguments[0] == NULL || strncmp(arguments[0], "#", 1) == 0) {}

		// The "exit" command exits the shell. 
		// The shell kill any other processes or jobs that the shell has started before it terminates itself.
		else if (strcmp(arguments[0], "exit") == 0) {
			int i;
			for (i = 0; i < pidsNumber; i++) {
				kill(backPids[i], SIGTERM);
			}
			exit(0);
		}

		// The "cd" command changes the working directory of the shell.
		else if (strcmp(arguments[0], "cd") == 0) {
			char* directory;

			// If the path is not specified, 
			// set the directory to what specified in the HOME environment variable.
			if (arguments[1] == NULL) {
				directory = getenv("HOME");
				chdir(directory);
			}

			// Else, set the directory to what specified by the user.
			else {
				directory = arguments[1];
			}

			// Change to the directory. If failed, print an error message.
			if (chdir(directory) != 0) {
				printf("%s: no such file or directory\n", directory);
				fflush(stdout);
			}
		}

		// The "status" command prints out either the exit status or the terminating signal of the last 
		// foreground process ran by the shell.
		else if (strcmp(arguments[0], "status") == 0) {
			// If the process exited normally, print out the exit status.
			if (WIFEXITED(status)) {
				printf("exit value %d\n", WEXITSTATUS(status));
				fflush(stdout);
			}

			// If the process was killed by a signal, print out the terminating signal.
			else {
				printf("terminated by signal %d\n", status);
				fflush(stdout);
			}

			fflush(stdout);
		}

		// If a non-built in command is received, have the parent fork() off a child.
		else {
			// If fork is successful, the value of pid will be 0 in the child, the child's pid in the parent.
			pid = fork();
			switch (pid) {
			case -1:	// If fork failed, print an error message.
				perror("fork() failed!");
				int i;
				for (i = 0; i < pidsNumber; i++) {
					kill(backPids[i], SIGTERM);
				}
				status = 1;
				break;

			case 0:	// Commands in the child.
				// If it's a foreground command, 
				// reset signal handler to complete all foreground commands in the child.
				if (!background) {
					sigintAction.sa_handler = SIG_DFL;	// Register SIG_DFL as the signal handler.
					sigaction(SIGINT, &sigintAction, NULL);	// Install the signal handler.
				}

				// If input redirection is needed, do it.
				if (inputFile != NULL) {
					// Input file redirected via stdin should be opened for reading only.
					targetFile = open(inputFile, O_RDONLY);

					// If cannot open the file for reading, 
					// print an error message and set the exit status to 1.
					if (targetFile == -1) {
						printf("cannot open %s for input\n", inputFile);
						fflush(stdout);
						exit(1);
					}

					// Use dup2() to set up the redirection, if failed, print an error message.
					else if (dup2(targetFile, 0) == -1) {
						perror("dup2");
						exit(1);
					}

					close(targetFile);
				}

				// If output redirection is needed, do it.
				if (outputFile != NULL) {
					// An output file redirected via stdout should be opened for writing only.
					// Truncate the file if it already exists or create the file if it does not exist.
					targetFile = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

					// If cannot open the output file, 
					// print an error message and set the exit status to 1.
					if (targetFile == -1) {
						printf("cannot open %s for output\n", outputFile);
						fflush(stdout);
						exit(1);
					}

					// Use dup2() to set up the redirection, if failed, print an error message.
					else if (dup2(targetFile, 1) == -1) {
						perror("dup2");
						exit(1);
					}

					close(targetFile);
				}

				// If it's a background command, the shell will not wait for background commands to complete.
				if (background) {
					// If the file to take standard input from is not sprcified,
					// have the standard input redirected from /dev/null.
					if (inputFile == NULL) {
						targetFile = open("/dev/null", O_RDONLY);	// Open /dev/null for reading only.

						// If cannot open the file for reading, 
						// print an error message and set the exit status to 1.
						if (targetFile == -1) {
							printf("cannot open /dev/null for input\n");
							fflush(stdout);
							exit(1);
						}

						// Use dup2() to set up the redirection, if failed, print an error message.
						else if (dup2(targetFile, 0) == -1) {
							perror("dup2");
							exit(1);
						}

						close(targetFile);
					}

					// If the file to send standard output to is not sprcified,
					// have the standard output redirected from /dev/null.
					if (outputFile == NULL) {
						targetFile = open("/dev/null", O_WRONLY);	// Open /dev/null for writing only.

						// If cannot open the file for reading, 
						// print an error message and set the exit status to 1.
						if (targetFile == -1) {
							printf("cannot open /dev/null for output\n");
							fflush(stdout);
							exit(1);
						}

						// Use dup2() to set up the redirection, if failed, print an error message.
						else if (dup2(targetFile, 1) == -1) {
							perror("dup2");
							exit(1);
						}

						close(targetFile);
					}
				}

				// Execute the command, if failed, print an error message.
				if (execvp(arguments[0], arguments)) {
					printf("%s: no such file or directory\n", arguments[0]);
					fflush(stdout);
					exit(1);
				}

			default:	// Commonds in the parent.
				// The shell should wait for completion of foreground commands,
				// before prompting for the next command.
				if (!background) {
					// Call waitpid() on the child to wait the child terminates.
					pid = waitpid(pid, &status, 0);
					if (WIFSIGNALED(status)) {
						printf("terminated by signal %d\n", status);
						fflush(stdout);
					}
				}

				// The shell will not wait for background commands to complete. 
				if (background) {
					waitpid(pid, &status, WNOHANG);
					backPids[pidsNumber] = pid;
					// Print the process id of a background process when it begins.
					printf("background pid is %d\n", pid);
					fflush(stdout);
					pidsNumber++;
				}
			}
		}

		// Clear up for the next input command.
		int i;
		for (i = 0; i < argumentsNumber; i++) {
			arguments[i] = NULL;
		}
		inputFile = NULL;
		outputFile = NULL;
		inputCommand = NULL;
		free(inputCommand); 

		// Periodically check for the background child processes to complete with waitpid(...NOHANG...) to
		// clean up all background child processes.
		pid = waitpid(-1, &status, WNOHANG);
		while (pid > 0) {
			// If the process exits normally, print a message showing the process id and exit value.
			if (WIFEXITED(status))
			{
				printf("background pid %d is done: exit value %d\n", pid, status);
				fflush(stdout);
			}

			// If the process was killed by a signal, print a message showing the process id and the number 
			// of the signal that killed the process.
			else {
				printf("background pid %d is done: terminated by signal %d\n", pid, status);
				fflush(stdout);
			}

			pid = waitpid(-1, &status, WNOHANG);	// Continue to the next checking.
		}
	}

	return 0;
}