#ifndef JOS_INC_STDIO_H
#define JOS_INC_STDIO_H

#include <inc/stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#define Black 30
#define Red 31
#define Green 32
#define Yellow 33
#define Blue 34
#define Magenta 35
#define Cyan 36
#define White 37

// lib/stdio.c
void	cputchar(int c);
int	getchar(void);
int	iscons(int fd);

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

// lib/printf.c
int	cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);

// lib/fprintf.c
int	printf(const char *fmt, ...);
int	fprintf(int fd, const char *fmt, ...);
int	vfprintf(int fd, const char *fmt, va_list);

// lib/readline.c
char*	readline(const char *prompt);

#endif /* !JOS_INC_STDIO_H */
