#include "builtin.h"
#include "utils.h"

// TODO actualizar el status
extern int status;

static char buffer[BUFLEN - 2] = { 0 };
static char *args[ARGSIZE] = { 0 };

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	// Your code here
	strcpy_until(buffer, cmd, ' ');
	return strcmp(buffer, "exit") == 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	// Your code here
	strcpy_until(buffer, cmd, ' ');
	if (strcmp(buffer, "cd") != 0)
		return 0;

	if (strcmp(cmd, "cd") == 0) {
		if (chdir(getenv("HOME")) != 0) {
			return status = 1;
		}
		if (getcwd(buffer, sizeof(buffer))) {
			return status = 1;
		}
		snprintf(prompt, PRMTLEN, "(%s)", buffer);
		return 1;
	}

	dispatch_args(args, cmd);
	if (chdir(args[1]) != 0) {
		return status = 1;
	}

	if (!getcwd(buffer, sizeof(buffer))) {
		return status = 1;
	}

	snprintf(prompt, PRMTLEN, "(%s)", buffer);
	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	// Your code here
	strcpy_until(buffer, cmd, ' ');
	if (strcmp(buffer, "pwd") != 0)
		return 0;

	if (!getcwd(buffer, sizeof(buffer))) {
		status = 1;
		return 1;
	}

	printf("%s\n", buffer);
	return 1;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here
	// return para evitar warning
	return strcmp(cmd, cmd);
}
