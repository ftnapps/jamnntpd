#include "nntpserv.h"

int main(int argc, char **argv) 
{ 
   SOCKET sock;
   int error,res,c; 
   struct sockaddr_in local;
   fd_set fds;
   struct timeval tv;

   if(argc == 2 && (stricmp(argv[1],"?")==0 || stricmp(argv[1],"-h")==0))
   {

           /*          1         2         3         4         5         6         7         8 */
           /* 12345678901234567890123456789012345678901234567890123456789012345678901234567890 */     
      printf("\n"
             "Usage: jamnntpd [<options>]\n"
             "\n"
             " General options:\n"
             "\n"
             " -p <port>             Port number for JamNNTPd (default: 5000)\n"
             " -m <maxcomm>          Maximum number of simultaneous connections (default: 5)\n"
             " -g <groupsfile>       Read this file instead of " CFG_GROUPSFILE "\n"
             " -a <allowfile>        Read this file instead of " CFG_ALLOWFILE "\n"
             " -u <usersfile>        Read this file instead of " CFG_USERSFILE "\n"
             " -x <xlatfile>         Read this file instead of " CFG_XLATFILE "\n"
             " -l <logfile>          Write to this file instead of " CFG_LOGFILE "\n"
             " -noecholog            Do not write log messages to console\n" 
             " -debug                Write all network communication to console\n"
             "\n"
             " Options for displaying messages:\n"
             "\n"
             " -readorigin           Get addresses from the origin line instead of JAM header\n"
             " -noencode             Do not MIME encode headers with 8-bit characters\n"
             " -strictnetmail        Use strict article counters in netmail areas\n"
             " -def_flowed on/off    Default setting for format=flowed (RFC 2646)\n"
             " -def_showto on/off    Default setting for the display of recipient on from line\n"
             "\n"
             " Options for posting messages:\n"
             "\n"
             " -nostripre            Do not remove \"Re:\" from subject line\n"
             " -notearline           Do not put X-Newsreader/User-Agent string on tearline\n"
             " -noreplyaddr          Do not create REPLYADDR kludges\n"
             " -notzutc              Do not create TZUTC kludges\n"
             " -nocancel             Do not allow cancelling of messages\n"
             " -smartquote           Reformat quoted text to fidonet style\n"
             " -origin <origin>      Put this on the Origin line instead of Organization\n"
             " -guestsuffix <suffix> Suffix added to from name of unauthenticated users\n"
             " -echomailjam <file>   Create echomail.jam file for CrashMail and other tossers\n"
             " -echotosslog <file>   Create echotoss log for hpt and other tossers\n"
             "\n");

      exit(0);
   }

   for(c=1;c<argc;c++)
   {
      if(stricmp(argv[c],"-debug")==0)
      {
         cfg_debug=TRUE;
      }
      else if(stricmp(argv[c],"-noecholog")==0)
      {
         cfg_noecholog=TRUE;
      }
      else if(stricmp(argv[c],"-nostripre")==0)
      {
         cfg_nostripre=TRUE;
      }
      else if(stricmp(argv[c],"-notearline")==0)
      {
         cfg_notearline=TRUE;
      }
      else if(stricmp(argv[c],"-nocancel")==0)
      {
         cfg_nocancel=TRUE;
      }
      else if(stricmp(argv[c],"-strictnetmail")==0)
      {
         cfg_strictnetmail=TRUE;
      }
      else if(stricmp(argv[c],"-readorigin")==0)
      {
         cfg_readorigin=TRUE;
      }
      else if(stricmp(argv[c],"-noreplyaddr")==0)
      {
         cfg_noreplyaddr=TRUE;
      }
      else if(stricmp(argv[c],"-smartquote")==0)
      {
         cfg_smartquote=TRUE;
      }
      else if(stricmp(argv[c],"-noencode")==0)
      {
         cfg_noencode=TRUE;
      }
      else if(stricmp(argv[c],"-notzutc")==0)
      {
         cfg_notzutc=TRUE;
      }
      else if(stricmp(argv[c],"-p")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_port=atoi(argv[++c]);
      }
      else if(stricmp(argv[c],"-m")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_maxconn=atoi(argv[++c]);
      }
      else if(stricmp(argv[c],"-def_flowed")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         if(!(setboolonoff(argv[c+1],&cfg_def_flowed)))
         {
            printf("Invalid setting %s for %s, must be on or off\n",argv[c+1],argv[c]);
            exit(0);
         }

         c++;
      }
      else if(stricmp(argv[c],"-def_showto")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         if(!(setboolonoff(argv[c+1],&cfg_def_showto)))
         {
            printf("Invalid setting %s for %s, must be on or off\n",argv[c+1],argv[c]);
            exit(0);
         }

         c++;
      }
      else if(stricmp(argv[c],"-origin")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_origin=argv[++c];
      }
      else if(stricmp(argv[c],"-guestsuffix")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_guestsuffix=argv[++c];
      }
      else if(stricmp(argv[c],"-echomailjam")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_echomailjam=argv[++c];
      }
      else if(stricmp(argv[c],"-echotosslog")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_echotosslog=argv[++c];
      }
      else if(stricmp(argv[c],"-g")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_groupsfile=argv[++c];
      }
      else if(stricmp(argv[c],"-a")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_allowfile=argv[++c];
      }
      else if(stricmp(argv[c],"-u")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_usersfile=argv[++c];
      }
      else if(stricmp(argv[c],"-x")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_xlatfile=argv[++c];
      }
      else if(stricmp(argv[c],"-l")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s\n",argv[c]);
            exit(0);
         }

         cfg_logfile=argv[++c];
      }
      else
      {
         printf("Unknown switch %s\n",argv[c]);
         exit(0);
      }
   }

   if(!os_init())
   {
       exit(10);
   }
	
   sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

   if(sock == INVALID_SOCKET)
   {
      uchar err[200];

      os_showerror("Failed to create socket: %s",os_strerr(os_errno(),err,200));
      os_free();
      exit(10);
   }

   memset(&local, 0, sizeof(local) );

   local.sin_family = AF_INET;
   local.sin_addr.s_addr = INADDR_ANY;
   local.sin_port = htons(cfg_port);

   error = bind(sock,(struct sockaddr *)&local,sizeof(local));

   if(error == SOCKET_ERROR)
   {
      uchar err[200];

      os_showerror("Could not bind to port (server already running?): %s",os_strerr(os_errno(),err,200));
      close(sock);
      os_free();

      exit(10);
   }

   error = listen(sock,5);

   if(error == SOCKET_ERROR)
   {
      uchar err[200];

      os_showerror("Could not listen to socket: %s",os_strerr(os_errno(),err,200));
      close(sock);
      os_free();
      exit(10);
   }

   os_logwrite(SERVER_NAME " " SERVER_VERSION " is running on port %d",cfg_port);

   if(cfg_debug)
      os_logwrite("Compiled " __DATE__ " " __TIME__);

   while(!get_server_quit())
   {
      FD_ZERO(&fds);
      FD_SET(sock,&fds);
	
      tv.tv_sec=1;
      tv.tv_usec=0;
	
      res=select(sock+1,&fds,NULL,NULL,&tv);

      if(res != 0 && res != -1)
      {
          SOCKET active_sock; 
          struct sockaddr_in from; 
          int fromlen = sizeof(from);               

          active_sock = accept(sock,(struct sockaddr *)&from,&fromlen);

          if(active_sock == SOCKET_ERROR)
          {
              uchar err[200];
              os_showerror("Failed to accept incoming connection: %s",os_strerr(os_errno(),err,200));
              break;
          }

          os_startserver(server,active_sock);
      }
   }

   if(get_server_openconnections())
   {
      os_logwrite("Exiting. %ld connection(s) are active, waiting for them to quit",
			get_server_openconnections());

      while(get_server_openconnections())
         os_sleep(1);
   }

   close(sock);

   os_logwrite(SERVER_NAME " exited");
   os_free();

   exit(0);
}

