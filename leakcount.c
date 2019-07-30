/*	Donald Elmore
	File Description:
		This file takes at minimum one argument:
			the test program to be ran
		This file takes in a test program from the user and execs it
			using execvpe, using LD_PRELOAD to load my version of malloc and free,
			along with passing the users arguments for their test program.

		The parent then waits for the child process to finish before returning.

	Known bugs:
		None known.
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
	
	//check that the user entered a target program
	if (argc < 2) {
		perror("Need 2 arguments: ./leakcount and ./a_test_program!");
	}

	extern char ** environ;
	pid_t pid = fork();
	switch (pid) {
		case 0:
			setenv("LD_PRELOAD", "./memory_shim.so", 1);
			execvpe(argv[1], argv + 1, environ);
			perror("execvpe error!");

			//should never reach here
			exit(1);
		case -1:
			perror("fork error!");
			exit(1);
		default:
			wait(NULL);
	}

	return 0;
}
