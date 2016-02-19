#ifndef THM_HEADER
#define THM_HEADER

typedef int (*thm_jft_t) (void *arg, void *ro, void **result);
typedef int (*thm_rft_t) (void *ro, void *arg, void *result);

int
thm_init(unsigned char len);

char
thm_is_inited();

unsigned char
thm_threads_cnt();

int
thm_addJob(void *ro, thm_jft_t jf, void *jarg, thm_rft_t rf);

#endif
