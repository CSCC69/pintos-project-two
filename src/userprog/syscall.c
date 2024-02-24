#include "userprog/syscall.h"
#include "devices/shutdown.h"
#include "stdio.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
stack_pop (void **syscall_args, int num_args, void *esp)
{
  for (int i = 0; i < num_args; i++)
    {
      syscall_args[i] = esp;
      esp += 4;
    }
}

static void
halt (void)
{
  shutdown_power_off ();
}

static void
exit (int status)
{
  printf ("%s: exit(%d)\n", thread_name (), status);
  thread_exit ();
}

static void
exec (const char *file)
{
  process_execute (file);
}

static void
wait (tid_t pid)
{
  process_wait (pid);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  void *esp = f->esp;
  int syscall_number = *(int *)esp;
  esp += sizeof (int);

  void *syscall_args[3];

  switch (syscall_number)
    {
    // 0 args
    case SYS_HALT:
      halt ();
      break;
    // 1 arg
    case SYS_EXIT:
      stack_pop (&syscall_args[0], 1, esp);
      int status = *(int *)syscall_args[0];
      exit (*(int *)syscall_args[0]);
      break;
    case SYS_EXEC:
      stack_pop (&syscall_args[0], 1, esp);
      const char *file = *(const char **)syscall_args[0];
      exec (file);
      break;
    case SYS_WAIT:
      stack_pop (&syscall_args[0], 1, esp);
      tid_t pid = *(tid_t *)syscall_args[0];
      wait (pid);
      break;
    case SYS_REMOVE:
      stack_pop (&syscall_args[0], 1, esp);
      break;
    case SYS_OPEN:
      stack_pop (&syscall_args[0], 1, esp);
      break;
    case SYS_FILESIZE:
      stack_pop (&syscall_args[0], 1, esp);
      break;
    case SYS_TELL:
      stack_pop (&syscall_args[0], 1, esp);
      break;
    case SYS_CLOSE:
      stack_pop (&syscall_args[0], 1, esp);
      break;
    // 2 args
    case SYS_CREATE:
      stack_pop (&syscall_args[0], 2, esp);
      break;
    case SYS_SEEK:
      stack_pop (&syscall_args[0], 2, esp);
      break;
    // 3 args
    case SYS_READ:
      stack_pop (&syscall_args[0], 3, esp);
      break;
    case SYS_WRITE:
      hex_dump((uintptr_t)PHYS_BASE-64, esp - (uint32_t)esp % 64, 64, true);
      stack_pop (&syscall_args[0], 3, esp);
      int fd = *(int *)syscall_args[0];
      const char *buffer = *(const char **)syscall_args[1];
      unsigned int length = *(unsigned int *)syscall_args[2];
      if (fd == STDOUT_FILENO)
        putbuf(buffer, length);
      break;
    default:
      break;
    }

  //printf ("system call! num: %d\n", syscall_number);
  thread_exit ();
}
