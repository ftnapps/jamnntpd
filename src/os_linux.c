#include "nntpserv.h"

#include <signal.h>

pthread_mutex_t linux_mutex = PTHREAD_MUTEX_INITIALIZER;

void sighandler(int sig)
{
   os_getexclusive();
   server_quit=TRUE;
   os_stopexclusive();
}

bool os_init(void)
{
   signal(SIGINT,sighandler);
   signal(SIGPIPE,SIG_IGN);
	
   return(TRUE);
}

void os_free(void)
{
   pthread_mutex_destroy(&linux_mutex);
}

void (*linux_srv)(SOCKET sock);

void *linux_serverstub(void *arglist)
{
   SOCKET s = (int) arglist;
   (*linux_srv)(s);
	return NULL;
}

void os_startserver(void (*srv)(SOCKET sock),SOCKET sock)
{
	pthread_t thr;
	pthread_attr_t attr;

	linux_srv=srv;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&thr,&attr,linux_serverstub,(void *)sock);
}

void os_sleep(int x)
{
   sleep(x);
}

void os_getexclusive(void)
{
   pthread_mutex_lock(&linux_mutex);
}

void os_stopexclusive(void)
{
	pthread_mutex_unlock(&linux_mutex);
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

   printf("JamNNTPd error: ");
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

