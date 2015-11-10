#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "lib/kernel/list.h"
#include "devices/input.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  if(f->esp==NULL || !is_user_vaddr(f->esp))
    syscall_exit(-1);
  
  // TODO: clean up this mess.
  // unknown issue: bit shifted 16 bits(3 param), 20 bits(4 param). don't know why.
  // issue: check 2 param normal?
  int syscall_number = *((int*)f->esp);
  switch(syscall_number)
  {
    case  SYS_HALT  : syscall_halt(); break;
    case  SYS_EXIT  : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      syscall_exit(*(int*)(f->esp+4)); break;
    case  SYS_EXEC  : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      f->eax = syscall_exec(*(const char**)(f->esp+4)); break;
    case  SYS_WAIT  : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      f->eax = syscall_wait(*(pid_t*)(f->esp+4)); break;
    case  SYS_CREATE: if(!is_user_vaddr(f->esp+8)) syscall_exit(-1);
                      f->eax = syscall_create(*(const char**) (f->esp+4),
                                              *(unsigned*) (f->esp+8)); break;
    case  SYS_REMOVE: if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      f->eax = syscall_remove(*(const char**)(f->esp+4)); break;
    case  SYS_OPEN  : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      f->eax = syscall_open(*(const char**)(f->esp+4)); break;
    case  SYS_FILESIZE  : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      f->eax = syscall_filesize(*(int*)(f->esp+4)); break;
    case  SYS_READ  : if(!is_user_vaddr(f->esp+16+12)) syscall_exit(-1);
                      f->eax = syscall_read(*(int*)(f->esp+16+4),
                                       *(void**)(f->esp+16+8),
                                       *(unsigned*)(f->esp+16+12)); break;
    case  SYS_WRITE : if(!is_user_vaddr(f->esp+16+12)) syscall_exit(-1);
                      f->eax = syscall_write(*(int*)(f->esp+16+4),
                                       *(const void**)(f->esp+16+8),
                                       *(unsigned*)(f->esp+16+12)); break;
    case  SYS_SEEK  : if(!is_user_vaddr(f->esp+8)) syscall_exit(-1);
                      syscall_seek(*(int*)(f->esp+4),
                                   *(unsigned*)(f->esp+8)); break;
    case  SYS_TELL  : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      f->eax = syscall_tell(*(int*)(f->esp+4)); break;
    case  SYS_CLOSE : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      syscall_close(*(int*)(f->esp+4)); break;
    case  SYS_FIBO  : if(!is_user_vaddr(f->esp+4)) syscall_exit(-1);
                      f->eax = syscall_fibonacci(*(int*)(f->esp+4)); break;
    case  SYS_SUM4  : if(!is_user_vaddr(f->esp+20+16)) syscall_exit(-1);
                      f->eax = syscall_sum_of_four_integers(*(int*)(f->esp+20+4),
                                                            *(int*)(f->esp+20+8),
                                                            *(int*)(f->esp+20+12),
                                                            *(int*)(f->esp+20+16)); break;
    default         : syscall_exit(-1);
  }
}

void
syscall_halt (void)
{
  // 3.3.4 description from 3.3.4 System Calls void halt(void).
  shutdown_power_off();
  return;
}

void
syscall_exit (int status)
{
  int i;
  struct thread *current = thread_current();
  char *name = current->name;

  // name has all the parameters too, so ignore others.
  for(i=0;i<16;++i)
    if(name[i]==' ') break;
  name[i] = '\0';

  // traverse through child list to exit for child before parent exit.
  struct list_elem *e;
  for(e=list_begin(&current->childlist); e!=list_end(&current->childlist); e = list_next(e))
    {
      struct thread *child = list_entry(e, struct thread, childelem);
      sema_up(&child->sema_wait);
    }

  // exit print.
  printf("%s: exit(%d)\n", current->name, status);
  current->is_done = true;
  current->return_status = status;
  // wait for parent to take over the result
  sema_down(&current->sema_wait);
  // call thread_exit to clean.
  thread_exit();
  return;
}

pid_t
syscall_exec (const char *cmdline)
{
  if(!is_user_vaddr(cmdline))
    syscall_exit(-1);
  return process_execute(cmdline);
}

int
syscall_wait (pid_t pid)
{
  return process_wait(pid);
}

bool
syscall_create (const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

bool
syscall_remove (const char *file)
{
  return filesys_remove(file);
}

int
syscall_open (const char *file)
{
  return filesys_open(file);
}

int
syscall_filesize (int fd)
{

}

int
syscall_read (int fd, void *buf, unsigned size)
{
  if(!is_user_vaddr(buf) || !is_user_vaddr(buf+size))
    syscall_exit(-1);
  // stdin
  if(fd==0)
    {
      unsigned i;
      for(i=0;i<size;++i)
        *((uint8_t*) buf + i) = input_getc();
      return size;
    }
  return -1;
}

int
syscall_write (int fd, const void *buf, unsigned size)
{
  if(!is_user_vaddr(buf) || !is_user_vaddr(buf+size))
    syscall_exit(-1);
  // stdout
  if(fd==1)
    {
      unsigned i;
      for(i=0;i<size;++i)
        if(!*((char*)(buf)+i)) break;
      putbuf(buf, i);
      return i;
    }
  return -1;
}

int
syscall_seek (int fd, unsigned position)
{

}

int
syscall_tell (int fd)
{

}

int
syscall_close (int fd)
{

}

int
syscall_fibonacci (int n)
{
  int f[3] = {0,1,0};
  int i;
  for(i=2;i<=n;++i)
    f[i%3] = f[(i+1)%3] + f[(i+2)%3];
  return f[n%3];
}

int
syscall_sum_of_four_integers (int a, int b, int c, int d)
{
  return a+b+c+d;
}
