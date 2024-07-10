#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };
static char handler_buf[BUFLEN] = { 0 };
void sig_handler(int);

// handler para cuando un proceso
// en segundo plano termina
void
sig_handler(int _)
{
	pid_t child_pid = _;
	if ((child_pid = waitpid(0, NULL, WNOHANG)) > 0) {
#ifndef SHELL_NO_INTERACTIVE
		snprintf(handler_buf,
		         sizeof(handler_buf),
		         "%s[PID=%i]%s\n$ ",
		         COLOR_BLUE,
		         child_pid,
		         COLOR_RESET);
#endif
		size_t len = strlen(handler_buf);
		_ = write(1, handler_buf, len * sizeof(char));
	}
}

// inicializa el nuevo stack para
// el handler de SIGCHLDs
static int
init_stack()
{
	void *stack = malloc(SIGSTKSZ);
	if (!stack) {
		perror("alt stack allocation");
		return 0;
	}

	stack_t new_stack = (stack_t){
		.ss_sp = stack,
		.ss_flags = 0,
		.ss_size = SIGSTKSZ,
	};

	if (sigaltstack(&new_stack, NULL) == -1) {
		perror("sigaltstack");
		return 0;
	}

	return SA_ONSTACK;
}

// inicializa el handling de señales de la shell
static void
init_signal_handling()
{
	// en caso de que no se inicialize el stack
	// alternativo se usa el mismo stack para
	// el handler de SIGCHLDs.
	struct sigaction sa = (struct sigaction){
		.sa_handler = sig_handler,
		.sa_flags = init_stack() | SA_RESTART,
	};

	sigfillset(&sa.sa_mask);
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
}


// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	init_signal_handling();
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
