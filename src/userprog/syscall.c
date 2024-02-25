#include "userprog/syscall.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "stdio.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "user/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <syscall-nr.h>
#include <string.h>

void syscall_handler (struct intr_frame *);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

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

void
syscall_handler (struct intr_frame *f)
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
      exit (status);
      break;
    case SYS_EXEC:
      stack_pop (&syscall_args[0], 1, esp);
      const char *file = *(const char **)syscall_args[0];
      f->eax = exec (file);
      break;
    case SYS_WAIT:
      stack_pop (&syscall_args[0], 1, esp);
      tid_t pid = *(tid_t *)syscall_args[0];
      f->eax = wait (pid);
      break;
    case SYS_REMOVE:
      stack_pop (&syscall_args[0], 1, esp);
      file = *(const char **)syscall_args[0];
      f->eax = remove (file);
      break;
    case SYS_OPEN:
      stack_pop (&syscall_args[0], 1, esp);
      file = *(const char **)syscall_args[0];
      f->eax = open (file);
      break;
    case SYS_FILESIZE:
      stack_pop (&syscall_args[0], 1, esp);
      int fd = *(int *)syscall_args[0];
      f->eax = filesize (fd);
      break;
    case SYS_TELL:
      stack_pop (&syscall_args[0], 1, esp);
      fd = *(int *)syscall_args[0];
      f->eax = tell (fd);
      break;
    case SYS_CLOSE:
      stack_pop (&syscall_args[0], 1, esp);
      fd = *(int *)syscall_args[0];
      close (fd);
      break;
    // 2 args
    case SYS_CREATE:
      stack_pop (&syscall_args[0], 2, esp);
      file = *(const char **)syscall_args[0];
      unsigned initial_size = *(unsigned *)syscall_args[1];
      f->eax = create (file, initial_size);
      break;
    case SYS_SEEK:
      stack_pop (&syscall_args[0], 2, esp);
      fd = *(int *)syscall_args[0];
      unsigned position = *(unsigned *)syscall_args[1];
      seek (fd, position);
      break;
    // 3 args
    case SYS_READ:
      stack_pop (&syscall_args[0], 3, esp);
      fd = *(int *)syscall_args[0];
      void *read_buffer = *(void **)syscall_args[1];
      unsigned size = *(unsigned *)syscall_args[2];
      f->eax = read (fd, read_buffer, size);
      break;
    case SYS_WRITE:
      stack_pop (&syscall_args[0], 3, esp);
      fd = *(int *)syscall_args[0];
      const char *write_buffer = *(const char **)syscall_args[1];
      unsigned int length = *(unsigned int *)syscall_args[2];
      f->eax = write (fd, write_buffer, length);
      break;
    default:
      break;
    }
}

void
halt (void)
{
  shutdown_power_off ();
}

void
exit (int status)
{
  struct thread *cur = thread_current ();
  cur->exit_status = status;
  printf ("%s: exit(%d)\n", cur->name, status);
  thread_exit ();
}

pid_t
exec (const char *cmd_line)
{
  if (cmd_line == NULL || strcmp(cmd_line, "") == 0)
    return PID_ERROR;
  pid_t pid = process_execute (cmd_line);
  sema_down(&get_thread_by_tid(pid)->exec_sema);
  return pid;
}

// TODO pid_t tid_t idk
int
wait (pid_t pid)
{
  if (pid < 0)
    return -1;
  return process_wait (pid);
}

bool
create (const char *file, unsigned initial_size)
{
  //TODO: FIX IT
  if(file == NULL)
    exit(-1);
  if (initial_size < 0 || strcmp(file, "") == 0 || strlen(file) > 14 || strlen(file) == 0 || strcmp(file, "\0") == 0)
    return false;
  return filesys_create (file, initial_size);
}

bool
remove (const char *file)
{
  if (file == NULL || strcmp(file, "") == 0)
    exit(-1);
  return filesys_remove (file);
}

int
open (const char *file)
{
  if (file == NULL || strcmp(file, "") == 0)
    return -1;
  struct file* opened_file = filesys_open(file);
  if (!opened_file)
    return -1;
  return add_fd_file(thread_current(), opened_file);
}

int
filesize (int fd)
{
  if (fd < 0)
    return -1;

  struct file* file = get_open_file(thread_current(), fd);
  if (!file)
    return -1;
  return file_length(file);
}

int
read (int fd, void *buffer, unsigned size)
{
  if (fd < 0)
    return -1;

  char *buf = (char*)buffer;
  if (fd == STDIN_FILENO) {
    for (unsigned i = 0; i < size; i++)
      buf[i] = input_getc();
    return size;
  }
  else {
    struct file* file = get_open_file(thread_current(), fd);
    if (!file)
      return -1;
    return file_read(file, buffer, size);
  }
}

int
write (int fd, const void *buffer, unsigned length)
{
  if (fd < 0 || buffer == NULL || length < 0)
    return -1;

  if (fd == STDOUT_FILENO)
    {
      int remaining = length % 5;
      putbuf(buffer, remaining);

      for (unsigned i = remaining; i < length; i += 5)
        putbuf(&buffer[i], 5);
  
      return length;
    }
  else
    {
      struct file* file = get_open_file(thread_current(), fd);
      if (!file)
        return -1;
      return file_write(file, buffer, length);
    }
}

void
seek (int fd, unsigned position)
{
  if (fd < 0 || position < 0)
    return;

  struct file* file = get_open_file(thread_current(), fd);
  if(!file)
    return;
  file_seek(file, position);
}

unsigned
tell (int fd)
{
  if (fd < 0)
    return 0;

  struct file* file = get_open_file(thread_current(), fd);
  if(file) {
    return file_tell(file);
  }
  return 0;
}

void
close (int fd)
{
  if (fd < 0)
    return;

  struct file* file = get_open_file(thread_current(), fd);
  file_close(file);
  remove_fd_file(thread_current(), fd);
}
