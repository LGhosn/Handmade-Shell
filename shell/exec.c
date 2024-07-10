#include "exec.h"


// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int index = block_contains(eargv[i], '=');
		if (index < 0)
			continue;

		char key[BUFLEN];
		char value[BUFLEN];
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, index);
		if (setenv(key, value, REPLACE_ENV) < 0)
			perror("error setting environment variable");
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (flags & O_CREAT) {
		return open(file, flags, S_IWUSR | S_IRUSR);
	}
	return open(file, flags);
}

static void
redir_stdout(struct execcmd *r)
{
	if (strlen(r->out_file) != 0) {
		int fd_out =
		        open_redir_fd(r->out_file,
		                      O_CLOEXEC | O_CREAT | O_WRONLY | O_TRUNC);
		if (fd_out < 0) {
			exit(-1);
		}
		if (dup2(fd_out, 1) < 0) {
			exit(-1);
		}
	}
}

static void
redir_stdin(struct execcmd *r)
{
	if (strlen(r->in_file) != 0) {
		int fd_in = open_redir_fd(r->in_file, O_CLOEXEC | O_RDONLY);
		if ((fd_in < 0) || (dup2(fd_in, 0) < 0))
			exit(-1);
	}
}

static void
redir_stderr(struct execcmd *r)
{
	if (strlen(r->err_file) != 0) {
		if (strcmp(r->err_file, "&1") == 0) {
			if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
				exit(-1);
		} else {
			int fd_err = open_redir_fd(r->err_file,
			                           O_CLOEXEC | O_CREAT |
			                                   O_WRONLY | O_TRUNC);
			if ((fd_err < 0) || (dup2(fd_err, 2) < 0))
				exit(-1);
		}
	}
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		//
		// Your code here
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		if (execvp(e->argv[0], e->argv) < 0) {
			perror("error executing command");
			_exit(EXIT_FAILURE);
		}
		break;

	case BACK: {
		// runs a command in background
		//
		// Your code here
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//
		// Your code here

		r = (struct execcmd *) cmd;
		redir_stdout(r);
		redir_stdin(r);
		redir_stderr(r);

		// Termino
		set_environ_vars(r->eargv, r->eargc);
		execvp(r->argv[0], r->argv);
		exit(EXIT_FAILURE);

		break;
	}

	case PIPE: {
		// pipes two commands
		//
		// Your code here
		p = (struct pipecmd *) cmd;
		int fd[2];

		if (pipe(fd) < 0) {
			perror("Error en pipe");
			_exit(EXIT_FAILURE);
		}

		pid_t left_fork = fork();

		if (left_fork == -1) {
			perror("Error en fork");
			_exit(EXIT_FAILURE);
		}

		if (left_fork == 0) {  // Proceso hijo
			close(fd[READ]);

			// Redirige la salida estándar al extremo de escritura de la tubería
			if (dup2(fd[WRITE], STDOUT_FILENO) == -1) {
				perror("Error en dup2");
				_exit(EXIT_FAILURE);
			}

			close(fd[WRITE]);
			exec_cmd(p->leftcmd);
		} else {  // Proceso padre
			close(fd[WRITE]);

			pid_t right_fork = fork();

			if (right_fork == -1) {
				perror("Error en fork");
				_exit(EXIT_FAILURE);
			}

			if (right_fork == 0) {  // Proceso hijo del proceso padre
				// Redirige la entrada estándar al extremo de lectura de la tubería
				if (dup2(fd[READ], STDIN_FILENO) == -1) {
					perror("Error en dup2");
					_exit(EXIT_FAILURE);
				}

				close(fd[READ]);
				exec_cmd(p->rightcmd);
			} else {  // Proceso padre
				close(fd[READ]);
				// Espera a que terminen los procesos hijos
				waitpid(left_fork, NULL, 0);
				waitpid(right_fork, NULL, 0);
				_exit(EXIT_SUCCESS);
			}
		}
		break;
	}
	default:
		fprintf(stderr, "Unknown command type\n");
		break;
	}
}
