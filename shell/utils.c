#include "utils.h"
#include <stdarg.h>

// mueve el puntero hasta encontrar
// un caracter que no sea un espacio.
void
lstrip(char **str)
{
	while (**str == ' ')
		(*str)++;
}

// guarda punteros al inicio de cada
// palabra en str.
void
dispatch_args(char *args[], char *str)
{
	size_t i = 0;
	while (1) {
		char *r = split_line(str, ' ');
		args[i++] = str;
		if (strlen(r) == 0) {
			args[i] = NULL;
			return;
		}

		str = r;
	}
}

// copia un string src a dst hasta
// encontrar el caracter c.
void
strcpy_until(char *dst, char *src, char c)
{
	size_t i = 0;
	while (src[i] != c && i < BUFLEN - 1) {
		dst[i] = src[i];
		i++;
	}

	dst[i] = '\0';
}

// splits a string line in two
// according to the splitter character
char *
split_line(char *buf, char splitter)
{
	int i = 0;

	while (buf[i] != splitter && buf[i] != END_STRING)
		i++;

	buf[i++] = END_STRING;

	while (buf[i] == SPACE)
		i++;

	return &buf[i];
}

// looks in a block for the 'c' character
// and returns the index in which it is, or -1
// in other case
int
block_contains(char *buf, char c)
{
	for (size_t i = 0; i < strlen(buf); i++)
		if (buf[i] == c)
			return i;

	return -1;
}

// Printf wrappers for debug purposes so that they don't
// show when shell is compiled in non-interactive way
int
printf_debug(char *format, ...)
{
#ifndef SHELL_NO_INTERACTIVE
	va_list args;
	va_start(args, format);
	int ret = vprintf(format, args);
	va_end(args);

	return ret;
#else
	return 0;
#endif
}

int
fprintf_debug(FILE *file, char *format, ...)
{
#ifndef SHELL_NO_INTERACTIVE
	va_list args;
	va_start(args, format);
	int ret = vfprintf(file, format, args);
	va_end(args);

	return ret;
#else
	return 0;
#endif
}
