#include "nntpserv.h"

#define MAXFILEARGS 100

char *fileargv[MAXFILEARGS];
int fileargc;

bool readargs(uchar *file);
void createconfig(uchar *file);

bool parseargs(int argc, char **argv,uchar *filename,ulong line)
{   
   uchar *arg,src[100],tmp[200];
   int c;
   
   src[0]=0;

   if(filename)
      sprintf(src," (%.95s line %ld)",filename,line);
   
   for(c=0;c<argc;c++)
   {
      arg=argv[c];
      
      if(arg[0] == '-' && arg[1] == '-')
         arg=&arg[1]; /* --option should be equivalent to -option */
         
      if(filename && arg[0] && arg[0] != '-')
      {
         /* Add dash if missing */
         strcpy(tmp,"-");
         mystrncpy(&tmp[1],arg,199);
         arg=tmp;
      }
      
      if(stricmp(arg,"-debug")==0)
      {
         cfg_debug=TRUE;
      }
      else if(stricmp(arg,"-noecholog")==0)
      {
         cfg_noecholog=TRUE;
      }
      else if(stricmp(arg,"-nostripre")==0)
      {
         cfg_nostripre=TRUE;
      }
      else if(stricmp(arg,"-notearline")==0)
      {
         cfg_notearline=TRUE;
      }
      else if(stricmp(arg,"-nocancel")==0)
      {
         cfg_nocancel=TRUE;
      }
      else if(stricmp(arg,"-strictnetmail")==0)
      {
         cfg_strictnetmail=TRUE;
      }
      else if(stricmp(arg,"-readorigin")==0)
      {
         cfg_readorigin=TRUE;
      }
      else if(stricmp(arg,"-noreplyaddr")==0)
      {
         cfg_noreplyaddr=TRUE;
      }
      else if(stricmp(arg,"-smartquote")==0)
      {
         cfg_smartquote=TRUE;
      }
      else if(stricmp(arg,"-noencode")==0)
      {
         cfg_noencode=TRUE;
      }
      else if(stricmp(arg,"-notzutc")==0)
      {
         cfg_notzutc=TRUE;
      }
      else if(stricmp(arg,"-p")==0 || stricmp(arg,"-port")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_port=atoi(argv[++c]);
      }
      else if(stricmp(arg,"-m")==0 || stricmp(arg,"-max")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_maxconn=atoi(argv[++c]);
      }
      else if(stricmp(arg,"-def_flowed")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         if(!(setboolonoff(argv[c+1],&cfg_def_flowed)))
         {
            printf("Invalid setting %s for %s, must be on or off%s\n",argv[c+1],argv[c],src);
            return(FALSE);
         }

         c++;
      }
      else if(stricmp(arg,"-def_showto")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         if(!(setboolonoff(argv[c+1],&cfg_def_showto)))
         {
            printf("Invalid setting %s for %s, must be on or off%s\n",argv[c+1],argv[c],src);
            return(FALSE);
         }

         c++;
      }
      else if(stricmp(arg,"-origin")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_origin=argv[++c];
      }
      else if(stricmp(arg,"-guestsuffix")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_guestsuffix=argv[++c];
      }
      else if(stricmp(arg,"-echomailjam")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_echomailjam=argv[++c];
      }
      else if(stricmp(arg,"-g")==0 || stricmp(arg,"-groups")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_groupsfile=argv[++c];
      }
      else if(stricmp(arg,"-a")==0 || stricmp(arg,"-allow")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_allowfile=argv[++c];
      }
      else if(stricmp(arg,"-u")==0 || stricmp(arg,"-users")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_usersfile=argv[++c];
      }
      else if(stricmp(arg,"-x")==0 || stricmp(arg,"-xlat")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_xlatfile=argv[++c];
      }
      else if(stricmp(arg,"-l")==0 || stricmp(arg,"-logfile")==0)
      {
         if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         cfg_logfile=argv[++c];
      }
      else if(stricmp(arg,"-config")==0)
      {
         if(filename)
         {
            printf("%s may only be used on commandline%s\n",argv[c],src);
            return(FALSE);
         }
         else if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         if(!readargs(argv[++c]))
            return(FALSE);
      }
      else if(stricmp(arg,"-create")==0)
      {
         if(filename)
         {
            printf("%s may only be used on commandline%s\n",argv[c],src);
            return(FALSE);
         }
         
         else if(c+1 == argc)
         {
            printf("Missing argument for %s%s\n",argv[c],src);
            return(FALSE);
         }

         createconfig(argv[++c]);
         return(FALSE);
      }
      else
      {
         printf("Unknown switch %s%s\n",argv[c],src);
         return(FALSE);
      }
   }

   return(TRUE);
}

bool readargs(uchar *file)
{
   FILE *fp;
   ulong line,firstarg,newargs,pos;
   uchar s[1000],w[200];
   
   if(!(fp=fopen(file,"r")))
   { 
      printf("Failed to open %s\n",file);
      return(FALSE);
   }

   line=0;

   while(fgets(s,999,fp))
   {
      line++;
      strip(s);

      if(s[0] != '#')
      {
         firstarg=fileargc;
         newargs=0;
         pos=0;
         
         while(getcfgword(s,&pos,w,200))
         {
            if(fileargc == MAXFILEARGS)
            {
               printf("Too many options in %s, max is %d\n",file,MAXFILEARGS);
               fclose(fp);
               return(FALSE);
            }         
            
            if(!(fileargv[fileargc++] = strdup(w)))
            {
               fclose(fp);
               return(FALSE);
            }
            
            newargs++;
         }
         
         if(newargs)
         {
            if(!(parseargs(newargs,&fileargv[firstarg],file,line)))
               return(FALSE);
         }
      }
   }

   fclose(fp);
   return(TRUE);
}

void createconfig(uchar *file)
{
   FILE *fp;
   
   if(!(fp=fopen(file,"w")))
   { 
      printf("Failed to open %s\n",file);
      return;
   }
   
   fprintf(fp,"# JamNNTPd configuration file\n");
   fprintf(fp,"port %lu\n",cfg_port);
   fprintf(fp,"max %lu\n",cfg_maxconn);
   fprintf(fp,"groups \"%s\"\n",cfg_groupsfile);
   fprintf(fp,"allow \"%s\"\n",cfg_allowfile);
   fprintf(fp,"users \"%s\"\n",cfg_usersfile);
   fprintf(fp,"xlat \"%s\"\n",cfg_xlatfile);
   fprintf(fp,"logfile \"%s\"\n",cfg_logfile);
   fprintf(fp,"%snoecholog\n",cfg_noecholog ? "" : "#");
   fprintf(fp,"%sdebug\n",cfg_debug ? "" : "#");
   fprintf(fp,"%sreadorigin\n",cfg_readorigin ? "" : "#");
   fprintf(fp,"%snoencode\n",cfg_noencode ? "" : "#");
   fprintf(fp,"%sstrictnetmail\n",cfg_strictnetmail ? "" : "#");
   fprintf(fp,"def_flowed %s\n",cfg_def_flowed ? "on" : "off");
   fprintf(fp,"def_showto %s\n",cfg_def_showto ? "on" : "off");
   fprintf(fp,"%snostripre\n",cfg_nostripre ? "" : "#");
   fprintf(fp,"%snotearline\n",cfg_notearline ? "" : "#");
   fprintf(fp,"%snoreplyaddr\n",cfg_noreplyaddr ? "" : "#");
   fprintf(fp,"%snotzutc\n",cfg_notzutc ? "" : "#");
   fprintf(fp,"%snocancel\n",cfg_nocancel ? "" : "#");
   fprintf(fp,"%ssmartquote\n",cfg_smartquote ? "" : "#");
   
   if(cfg_origin) fprintf(fp,"origin \"%s\"\n",cfg_origin);
   else           fprintf(fp,"#origin <origin>\n");
   
   if(cfg_guestsuffix) fprintf(fp,"guestsuffix \"%s\"\n",cfg_guestsuffix);
   else                fprintf(fp,"#guestsuffix <suffix>\n");
   
   if(cfg_echomailjam) fprintf(fp,"echomailjam \"%s\"\n",cfg_echomailjam);
   else                fprintf(fp,"#echomailjam <file>\n");
   
   fclose(fp);
}

void freeargs(void)
{
   int c;

   for(c=0;c<fileargc;c++)
      free(fileargv[c]);
}

int main(int argc, char **argv) 
{ 
   SOCKET sock;
   int error,res; 
   struct sockaddr_in local;
   fd_set fds;
   struct timeval tv;
   FILE *fp;
   struct _minf m;
   
   if(argc == 2 && (stricmp(argv[1],"?")==0 || stricmp(argv[1],"-h")==0 || stricmp(argv[1],"--help")==0))
   {

           /*          1         2         3         4         5         6         7         8 */
           /* 12345678901234567890123456789012345678901234567890123456789012345678901234567890 */     
      printf("\n"
             "Usage: jamnntpd [<options>]\n"
             "\n"
             " General options:\n"
             "\n"
             " -p[ort] <port>         Port number for JamNNTPd (default: 5000)\n"
             " -m[ax] <maxconn>       Maximum number of simultaneous connections (default: 5)\n"
             " -g[roups] <groupsfile> Read this file instead of " CFG_GROUPSFILE "\n"
             " -a[llow] <allowfile>   Read this file instead of " CFG_ALLOWFILE "\n"
             " -u[sers] <usersfile>   Read this file instead of " CFG_USERSFILE "\n"
             " -x[lat] <xlatfile>     Read this file instead of " CFG_XLATFILE "\n"
             " -l[ogfile] <logfile>   Log to this file instead of " CFG_LOGFILE "\n"
             " -noecholog             Do not write log messages to console\n" 
             " -debug                 Write all network communication to console\n"
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
             " -origin <origin>      Origin to use instead of contents of Organization line\n"
             " -guestsuffix <suffix> Suffix added to from name of unauthenticated users\n"
             " -echomailjam <file>   Create echomail.jam file for CrashMail and other tossers\n"
             "\n"
             " Options for configuration files:\n"
             "\n"
             " -config <file>        Read options from this file\n"
             " -create <file>        Create a configuration file with the default options\n"
             "\n"
             "If no options are specified on the commandline, JamNNTPd will attempt to read\n"
             "options from " CONFIGFILE " (if it exists).\n"
             "\n");
      
      return(FALSE);
   }

   if(argc == 1)
   {
      if((fp=fopen(CONFIGFILE,"r")))
      {
         fclose(fp);
     
         if(!readargs(CONFIGFILE))
         {
            freeargs();
            exit(0);
         }
      }
   }
   else
   {
      if(!parseargs(argc-1,&argv[1],NULL,0))
      { 
         freeargs();
         exit(0);
      }
   }

   if(!os_init())
   {
       freeargs();
       exit(10);
   }
	
   m.req_version = 0;
   m.def_zone = 0;

   if(MsgOpenApi(&m) != 0)
   {
      os_showerror("Failed to initialize SMAPI");
      freeargs();
      os_free();
      exit(10);
   }

   sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

   if(sock == INVALID_SOCKET)
   {
      uchar err[200];

      os_showerror("Failed to create socket: %s",os_strerr(os_errno(),err,200));
      MsgCloseApi();
      os_free();
      freeargs();
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
      MsgCloseApi();
      os_free();
      freeargs();
      exit(10);
   }

   error = listen(sock,5);

   if(error == SOCKET_ERROR)
   {
      uchar err[200];

      os_showerror("Could not listen to socket: %s",os_strerr(os_errno(),err,200));
      close(sock);
      MsgCloseApi();
      os_free();
      freeargs();
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
   MsgCloseApi();
   os_free();
   freeargs();
   exit(0);
}

