#include "nntpserv.h"

struct sockio *allocsockio(SOCKET socket)
{
	struct sockio *sio;
	
	if(!(sio=(struct sockio *)malloc(sizeof(struct sockio))))
		return(NULL);

	sio->bufpos=0;
	sio->buflen=0;

	sio->socket=socket;

	return(sio);		
}

void freesockio(struct sockio *sio)
{
	free(sio);
}

int sockreadchar(struct sockio *sio)
{
	fd_set fds;
	struct timeval tv;
	int res,num;

	if(get_server_quit())
		return(-1);
		
	if(sio->bufpos < sio->buflen)
		return(sio->buf[sio->bufpos++]);

	num=0;

	for(;;)
	{
 		FD_ZERO(&fds);
		FD_SET(sio->socket,&fds);
	
		tv.tv_sec=1;
		tv.tv_usec=0;
	
		num++;
		res=select(sio->socket+1,&fds,NULL,NULL,&tv);

		if(res == -1)
		{
			return(-1);
		}
		else if(res)
		{
	      res=recv(sio->socket,sio->buf,SOCKIO_BUFLEN,0);
			
			if(res <= 0)
         {
				return(-1);
         }

			sio->buflen=res;
			sio->bufpos=0;
			
			return(sio->buf[sio->bufpos++]);
		}
		
      if(num == SOCKIO_TIMEOUT || get_server_quit())
			return(-1);
	}
}

bool sockreadline(struct var *var,uchar *buf,int len)
{
   int ch,d;

   d=0;

   for(;;)
   {
      ch=sockreadchar(var->sio);

      if(ch == -1)
      {
         var->disconnect=TRUE;
         return(FALSE);
      }

		buf[d++]=ch;
		
      if(ch == 10 || d == len-1)
      {
         buf[d]=0;
         return(TRUE);
      }
   }
}

void socksendtext(struct var *var,uchar *buf)
{
   if(var->disconnect)
      return;

   if(cfg_debug)
      printf("(%s) > %s",var->clientid,buf);

   if(send(var->sio->socket,buf,strlen(buf),0) == -1)
   {
      uchar err[100];

      os_strerr(os_errno(),err,100);

      os_logwrite("(%s) Socket error \"%s\" (%lu), disconnecting",var->clientid,err,os_errno());
      var->disconnect=TRUE;
   }
}

void sockprintf(struct var *var,uchar *fmt,...)
{
   va_list args;
   uchar buf[1000];

   va_start(args, fmt);
   vsprintf(buf,fmt,args);
   va_end(args);

   socksendtext(var,buf);
}
