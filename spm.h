#ifndef SPM_HEADER
#define SPM_HEADER

typedef struct str_st *str_t;

int
spm_exec(char *cmd, char *argv[], char *dir, int *fdin, int *fdout, int *fderr);

int
spm_wait(int pid, int *status);

int
spm_exec_wo(char *cmd, char *argv[], char *dir, int *st, str_t output);

int
spm_exec_w(char *cmd, char *argv[], char *dir, int *st);

#endif
