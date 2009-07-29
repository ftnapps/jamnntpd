#include "nntpserv.h"

#define INCL_DOSPROCESS 
#define INCL_DOSSEMAPHORES
#define INCL_DOSMODULEMGR
#include <os2.h>

#define THREAD_STACK_SIZE 65536

#include <signal.h>

HMTX os2_mutex;
HMODULE so32dll;
int (*so32dll_select)(int *fds,int numread,int numwrite,int numexcept,long timeout);

void sighandler(int sig)
{
   os_getexclusive();
   server_quit=TRUE;
   os_stopexclusive();
}

bool os_init(void)
{
   uchar objectname[40];

   if(DosLoadModule(objectname,sizeof(objectname),"SO32DLL",&so32dll) != 0)
   {         
      printf("Failed to load SO32DLL.DLL (Networking not installed?)\n");
      return(FALSE);
   }

   if(DosQueryProcAddr(so32dll,0,"SELECT",&so32dll_select) != 0)
   {
      printf("SELECT function not found in SO32DLL.DLL (Wrong version of SO32DLL.DLL?)\n");
      return(FALSE);
   }

   if(DosCreateMutexSem(NULL,&os2_mutex,0,FALSE) != 0)
   {
      printf("Failed to create mutex semaphore\n");
      return(FALSE);
   }

   signal(SIGINT,sighandler);
   signal(SIGPIPE,sighandler);
	
   return(TRUE);
}

void os_free(void)
{
   DosCloseMutexSem(os2_mutex);
   DosFreeModule(so32dll);
}

void (*os2_srv)(SOCKET sock);

void os2_serverstub(void *arglist)
{
   SOCKET s = (SOCKET) arglist;
   (*os2_srv)(s);
}

void os_startserver(void (*srv)(SOCKET sock),SOCKET sock)
{
   int error;

   os2_srv=srv;

   error=_beginthread(os2_serverstub,NULL,THREAD_STACK_SIZE,(void *)sock);

   if(error == -1)
      os_logwrite("Failed to accept incoming connection (_beginthread failed)");
}

void os_sleep(int x)
{
   DosSleep(x*1000);
}

void os_getexclusive(void)
{
   DosRequestMutexSem(os2_mutex,SEM_INDEFINITE_WAIT);
}

void os_stopexclusive(void)
{
   DosReleaseMutexSem(os2_mutex);
}

void os_logwrite(uchar *fmt,...)
{
   FILE *logfp;
   time_t t;
   struct tm *tp;
   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};
   uchar logline[500];
   va_list args;

   time(&t);
   tp=localtime(&t);

   sprintf(logline,"%02d-%s-%02d %02d:%02d:%02d ",
      tp->tm_mday,
      monthnames[tp->tm_mon],
      tp->tm_year%100,
      tp->tm_hour,
      tp->tm_min,
      tp->tm_sec);

   va_start(args, fmt);
   vsprintf(&logline[strlen(logline)],fmt,args);
   va_end(args);

   if(!cfg_noecholog)
      puts(logline);

   strcat(logline,"\n");

   if(!(logfp=fopen(cfg_logfile,"a")))
   {
      os_showerror("Failed to open logfile %s",cfg_logfile);
      return;
   }

   fputs(logline,logfp);
   fclose(logfp);
}

void os_showerror(uchar *fmt,...)
{
   va_list args;

   va_start(args, fmt);

   vprintf(fmt,args);
   printf("\n");

   va_end(args);
}

int os_errno(void)
{
   return errno;
}

uchar *os_strerr(int err,uchar *str,ulong len)
{
   mystrncpy(str,strerror(err),len);
   return(str);
}

/* The EMX implementation of select() cannot be called from multiple threads simultaneously.
    Thanks to Darren Abbott for pointing out this workaround in the emx mailing list. */

int os2_select(int n,fd_set *readfds,fd_set *writefds,fd_set *exceptfds,struct timeval *timeout)
{
   int os2_fds,ret;

   if(!FD_ISSET(n-1,readfds) || writefds || exceptfds)
      return(-1); /* This is not a full implementation nor does it aim to be */

   os2_fds=_getsockhandle(n-1);

   ret=so32dll_select(&os2_fds,1,0,0,timeout->tv_sec*1000);

   if(ret>0 && os2_fds<0) 
      FD_CLR(n-1,readfds);

   return(ret);
}
