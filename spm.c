#include "spm.h"
#include "objtypes.h"
#include "trace.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int
spm_exec(char *cmd, char *argv[], char *dir, int *fdin, int *fdout, int *fderr) {
  int pid, p1[2], p2[2], p3[2], ret;

  ret = pipe(p1);
  ASSERT(ret, "pipe");
  ret = pipe(p2);
  ASSERT(ret, "pipe");
  ret = pipe(p3);
  ASSERT(ret, "pipe");

  pid = fork();
  ASSERT(pid<0, "fork");
  if(!pid) { // child
    close(p1[1]);
    close(p2[0]);
    close(p3[0]);
    dup2(p1[0], STDIN_FILENO);
    dup2(p2[1], STDOUT_FILENO);
    dup2(p3[1], STDERR_FILENO);
    if(dir) {
      ret = chdir(dir);
      if(ret) {
	PERR("chdir");
	exit(-1);
      }
    }
    ret = execvp(cmd, argv);
    if(ret) {
      PERR("has not executable file, is it service?\n");
      exit(-1);
    }
    exit(0);
  } else { // parent
    close(p1[0]);
    close(p2[1]);
    close(p3[1]);
    *fdin = p1[1];
    *fdout = p2[0];
    *fderr = p3[0];
  }

  return pid;
 error:
  return -1;
}

int
spm_wait(int pid, int *status) {
  int stval, st, ret;

  ret = waitpid(pid, &stval, 0);
  ASSERT(ret<0, "waitpid");

  if(WIFEXITED(stval)) {
    st = WEXITSTATUS(stval);
    if(st > 128)
      st = st - 256;
    *status = st;
  } else {
    *status = -1;
  }

  return 0;
 error:
  return -1;
}

int
spm_exec_wo(char *cmd, char *argv[], char *dir, int *st, str_t output) {
  char buf[256];
  int pid, fi, fo, fe, rc, ret;

  pid = spm_exec(cmd, argv, dir, &fi, &fo, &fe);
  ASSERT(pid<0, "spm_exec");

  while((rc = read(fo, buf, 255)) > 0) {
    ret = str_add(output, buf, rc);
    ASSERT(ret, "str_add");
  }

  while((rc = read(fe, buf, 255)) > 0) {
    ret = str_add(output, buf, rc);
    ASSERT(ret, "str_add");
  }

  ret = spm_wait(pid, st);
  ASSERT(ret, "spm_wait");

  close(fi);
  close(fo);
  close(fe);

  return 0;
 error:
  return -1;
}

int
spm_exec_w(char *cmd, char *argv[], char *dir, int *st) {
  int pid, fi, fo, fe, ret;

  pid = spm_exec(cmd, argv, dir, &fi, &fo, &fe);
  ASSERT(pid<0, "spm_exec");

  ret = spm_wait(pid, st);
  ASSERT(ret, "spm_wait");

  close(fi);
  close(fo);
  close(fe);

  return 0;
 error:
  return -1;
}
