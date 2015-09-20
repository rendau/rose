#ifndef TRACE_HEADER
#define TRACE_HEADER

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define OTMR "\e[1;31m"
#define OTMG "\e[1;32m"
#define OTMY "\e[1;33m"
#define OTMB "\e[1;34m"
#define OTMM "\e[1;35m"
#define OTMC "\e[1;36m"
#define OTM  "\e[m"

#define TR(...) do{fprintf(stdout, __VA_ARGS__); fflush(stdout);}while(0)
#define VTR(fmt,args) vfprintf(stdout, fmt, args);
#define TRC(...) do{fprintf(stdout,OTMY); fprintf(stdout,__VA_ARGS__); fprintf(stdout,OTM); fflush(stdout);}while(0)

#define PERR(...) do{fprintf(stderr,OTMR"%s(%d):\n\t\t%s(): ",__FILE__,__LINE__,__FUNCTION__);fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\n\t\terror(%d): %s"OTM"\n",errno,strerror(errno));}while(0)

#define PWAR(...) do{fprintf(stderr,OTMR); fprintf(stderr,__VA_ARGS__); fprintf(stderr,OTM); fflush(stderr);}while(0)

#define ASSERT(cond,...) if(cond){PERR(__VA_ARGS__); goto error;}

#endif
