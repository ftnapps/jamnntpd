#include "nntpserv.h"

#include <signal.h>

CRITICAL_SECTION win32_critsec;

void sighandler(int sig)
{
   os_getexclusive();
   server_quit=TRUE;
   os_stopexclusive();
}

bool os_init(void)
{
   WSADATA wsaData; 
   int error;
	
   signal(SIGINT,sighandler);

   error = WSAStartup(MAKEWORD(1,1),&wsaData);

   if(error)
   {
      os_showerror("Failed to initialize Winsock (error: %d)",error);
      return(FALSE);
   }

   InitializeCriticalSection(&win32_critsec);

   return(TRUE);
}

void os_free(void)
{
   DeleteCriticalSection(&win32_critsec);
   WSACleanup();
}

void (*win32_srv)(SOCKET sock);

void _cdecl win32_serverstub(void *arglist)
{
   SOCKET s = (SOCKET) arglist;
   (*win32_srv)(s);
}

void os_startserver(void (*srv)(SOCKET sock),SOCKET sock)
{
   int error;

   win32_srv=srv;

   error=_beginthread(win32_serverstub,0,(void *)sock);

   if(error == -1)
      os_logwrite("Failed to accept incoming connection (_beginthread failed)");
}

void os_getexclusive(void)
{
   EnterCriticalSection(&win32_critsec);
}

void os_stopexclusive(void)
{
   LeaveCriticalSection(&win32_critsec);
}

void os_sleep(int x)
{
   sleep(x*1000);
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
	return WSAGetLastError();
}

uchar *os_strerr(int err,uchar *str,ulong len)
{
   sprintf(str,"Winsock error %d",err);

   return(str);
}

