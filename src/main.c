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
      printf("Usage: jamnntpd [-debug] [-noecholog] [-nostripre] [-notearline] [-nocancel]\n"
             "                [-noreplyaddr] [-smartquote] [-noencode] [-notzutc] [-p <port>]\n"
             "                [-m <maxconn>] [-def_flowed on/off] [-def_showto on/off]\n"
             "                [-origin <origin>] [-guestsuffix <suffix>]\n"
             "                [-echomailjam <echomail.jam> [-g <groupsfile>] [-a <allowfile>]\n"
             "                [-u <usersfile>] [-x <xlatfile>] [-l <logfile>]\n");

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
      uchar err[100];

      os_showerror("Could not create socket: %s",os_strerr(os_errno(),err,100));

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
      uchar err[100];

		os_showerror("Error in bind: %s (Server already running?)",os_strerr(os_errno(),err,100));

      close(sock);
      os_free();

      exit(10);
   }

   error = listen(sock,5);

   if(error == SOCKET_ERROR)
   {
      uchar err[100];

		os_showerror("Error in listen: %s",os_strerr(os_errno(),err,100));

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
              uchar err[100];

				  os_showerror("accept() failed: %s\n",os_strerr(os_errno(),err,100));
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

