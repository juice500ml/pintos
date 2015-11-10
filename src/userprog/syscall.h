#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"
#include <stdint.h>
#include <stdbool.h>

void syscall_init (void);

// 3.3.4 system calls implementation
int read_byte (const uint8_t *uaddr);
bool write_byte (uint8_t *udst, uint8_t byte);

void syscall_halt (void);
void syscall_exit (int status);
pid_t syscall_exec (const char *cmdline);
int syscall_wait (pid_t pid);
bool syscall_create (const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
int syscall_open (const char *file);
int syscall_filesize (int fd);
int syscall_read (int fd, void *buf, unsigned size);
int syscall_write (int fd, const void *buf, unsigned size);
int syscall_seek (int fd, unsigned position);
int syscall_tell (int fd);
int syscall_close (int fd);
int syscall_fibonacci (int n);
int syscall_sum_of_four_integers (int a, int b, int c, int d);

#endif /* userprog/syscall.h */
