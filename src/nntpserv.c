#include "nntpserv.h"

ulong cfg_port        = CFG_PORT;
ulong cfg_maxconn     = CFG_MAXCONN;

uchar *cfg_origin;
uchar *cfg_guestsuffix;
uchar *cfg_echomailjam;

uchar *cfg_allowfile  = CFG_ALLOWFILE;
uchar *cfg_groupsfile = CFG_GROUPSFILE;
uchar *cfg_logfile    = CFG_LOGFILE;
uchar *cfg_usersfile  = CFG_USERSFILE;
uchar *cfg_xlatfile   = CFG_XLATFILE;

bool cfg_def_flowed = CFG_DEF_FLOWED;
bool cfg_def_showto = CFG_DEF_SHOWTO;

bool cfg_debug;
bool cfg_noecholog;
bool cfg_nostripre;
bool cfg_noreplyaddr;
bool cfg_notearline;
bool cfg_smartquote;
bool cfg_noencode;
bool cfg_notzutc;

int server_openconnections;
int server_quit;

ulong lastmsgidnum;

int get_server_openconnections(void)
{
   int res;

   os_getexclusive();
   res=server_openconnections;
   os_stopexclusive();

   return(res);
}

int get_server_quit(void)
{
   int res;

   os_getexclusive();
   res=server_quit;
   os_stopexclusive();

   return(res);
}

ulong get_msgid_num(void)
{
   ulong msgidnum;

   os_getexclusive();

   msgidnum=(time(NULL)/10)*10;

   if(msgidnum <= lastmsgidnum)
      msgidnum = lastmsgidnum+1;

   lastmsgidnum=msgidnum;

   os_stopexclusive();

   return(msgidnum);
}

uchar *parseinput(struct var *var)
{
   long s;

	/* Skip whitespace */
   while(var->input[var->inputpos]==' ')
      var->inputpos++;

   s=var->inputpos;

   if(var->input[var->inputpos] == 0)
      return(NULL);

   while(var->input[var->inputpos]!=' ' && var->input[var->inputpos]!=0)
      var->inputpos++;

   if(var->input[var->inputpos] == ' ')
      var->input[var->inputpos++]=0;

   return(&var->input[s]);
}

bool jamopenarea(struct var *var,struct group *group)
{
   if(group == var->opengroup)
      return(TRUE);

   if(var->openmb)
   {
      JAM_CloseMB(var->openmb);
      free(var->openmb);
      var->openmb=NULL;
      var->opengroup=NULL;
   }

   if(JAM_OpenMB(group->jampath,&var->openmb))
   {
      if(var->openmb)
      {
         free(var->openmb);
         var->openmb=NULL;
         var->opengroup=NULL;
      }
      
      os_logwrite("(%s) Failed to open JAM messagebase \"%s\"",var->clientid,group->jampath);
      return(FALSE);
   }

   var->opengroup=group;

   return(TRUE);
}

bool jamgetminmaxnum(struct var *var,struct group *group,ulong *min,ulong *max,ulong *num)
{
   s_JamBaseHeader Header_S;

   if(!(jamopenarea(var,group)))
      return(FALSE);

   if(JAM_GetMBSize(var->openmb,num))
   {
      os_logwrite("(%s) Failed to get size of JAM area \"%s\"",var->clientid,group->jampath);
      return(FALSE);
   }

   if(JAM_ReadMBHeader(var->openmb,&Header_S))
   {
      os_logwrite("(%s) Failed to read header of JAM area \"%s\"",var->clientid,group->jampath);
      return(FALSE);
   }

   if(*num)
   {
      *min=Header_S.BaseMsgNum;
      *max=Header_S.BaseMsgNum+*num-1;
   }
   else
   {
      *min=0;
      *max=0;
   }

   return(TRUE);
}

void command_list(struct var *var)
{
   struct group *g;
   ulong min,max,num;
   uchar *arg;

   arg=parseinput(var);

   if(arg)
   {
      if(stricmp(arg,"overview.fmt") == 0)
      {
         socksendtext(var,"215 List of fields in XOVER result" CRLF);
         socksendtext(var,"Subject:" CRLF);
         socksendtext(var,"From:" CRLF);
         socksendtext(var,"Date:" CRLF);
         socksendtext(var,"Message-ID:" CRLF);
         socksendtext(var,"References:" CRLF);
         socksendtext(var,"Bytes:" CRLF),
         socksendtext(var,"Lines:" CRLF);
         socksendtext(var,"." CRLF);

         return;
      }
      else if(stricmp(arg,"active") != 0)
      {
         socksendtext(var,"501 Unknown argument for LIST command" CRLF);
         return;
      }
   }

   socksendtext(var,"215 List of newsgroups follows" CRLF);

   for(g=var->firstgroup;g && !var->disconnect && !get_server_quit();g=g->next)
   {
      if(matchgroup(var->readgroups,g->group))
      {
         if(!jamgetminmaxnum(var,g,&min,&max,&num))
         {
            min=0;
            max=0;
            num=0;
         }

         if(matchgroup(var->postgroups,g->group))
            sockprintf(var,"%s %lu %lu y" CRLF,g->tagname,min,max);

         else
            sockprintf(var,"%s %lu %lu n" CRLF,g->tagname,min,max);
      }
   }

   socksendtext(var,"." CRLF);
}

void command_group(struct var *var)
{
   uchar *groupname;
   struct group *g;
   ulong min,max,num;

   if(!(groupname=parseinput(var)))
   {
      socksendtext(var,"501 No group specified" CRLF);
      return;
   }

   for(g=var->firstgroup;g;g=g->next)
      if(matchgroup(var->readgroups,g->group) && stricmp(g->tagname,groupname)==0) break;

   if(!g)
   {
      socksendtext(var,"411 No such newsgroup" CRLF);
      return;
   }

   if(!jamgetminmaxnum(var,g,&min,&max,&num))
   {
      socksendtext(var,"503 Local error: Could not get size of messagebase" CRLF);
      return;
   }

   var->currentgroup=g;
   var->currentarticle=min;

   sockprintf(var,"211 %lu %lu %lu %s Group selected" CRLF,num,min,max,g->tagname);
}

void command_next(struct var *var)
{
   ulong min,max,num;

   if(!var->currentgroup)
   {
      socksendtext(var,"412 No newsgroup selected" CRLF);
      return;
   }

   if(!var->currentarticle)
   {
      socksendtext(var,"420 No current article has been selected" CRLF);
      return;
   }

   if(!jamgetminmaxnum(var,var->currentgroup,&min,&max,&num))
   {
      socksendtext(var,"503 Local error: Could not get size of messagebase" CRLF);
      return;
   }

   if(var->currentarticle+1 > max)
   {
      socksendtext(var,"421 No next article in this group" CRLF);
      return;
   }

   var->currentarticle++;

   sockprintf(var,"223 %lu <%lu$%s@JamNNTPd> Article retrieved" CRLF,
      var->currentarticle,var->currentarticle,var->currentgroup->tagname);
}

void command_last(struct var *var)
{
   ulong min,max,num;

   if(!var->currentgroup)
   {
      socksendtext(var,"412 No newsgroup selected" CRLF);
      return;
   }

   if(!var->currentarticle)
   {
      socksendtext(var,"420 No current article has been selected" CRLF);
      return;
   }

   if(!jamgetminmaxnum(var,var->currentgroup,&min,&max,&num))
   {
      socksendtext(var,"503 Local error: Could not get size of messagebase" CRLF);
      return;
   }

   if(var->currentarticle-1 < min)
   {
      socksendtext(var,"422 No previous article in this group" CRLF);
      return;
   }

   var->currentarticle--;

   sockprintf(var,"223 %lu <%lu$%s@JamNNTPd> Article retrieved" CRLF,
      var->currentarticle,var->currentarticle,var->currentgroup->tagname);
}

struct attributename
{
   ulong attr;
   uchar *name;
};

struct attributename attributenames[] =
{ { MSG_LOCAL,       "Local"        },
  { MSG_INTRANSIT,   "InTransit"    },
  { MSG_PRIVATE,     "Private"      },
  { MSG_READ,        "Read"         },
  { MSG_SENT,        "Sent"         },
  { MSG_KILLSENT,    "KillSent"     },
  { MSG_ARCHIVESENT, "ArchiveSent"  },
  { MSG_HOLD,        "Hold"         },
  { MSG_CRASH,       "Crash"        },
  { MSG_IMMEDIATE,   "Immediate"    },
  { MSG_DIRECT,      "Direct"       },
  { MSG_GATE,        "Gate"         },
  { MSG_FILEREQUEST, "FReq"         },
  { MSG_FILEATTACH,  "FAttach"      },
  { MSG_TRUNCFILE,   "TruncFile"    },
  { MSG_KILLFILE,    "KillFile"     },
  { MSG_RECEIPTREQ,  "ReceiptReq"   },
  { MSG_CONFIRMREQ,  "ConfirmReq"   },
  { MSG_ORPHAN,      "Orphan"       },
  { MSG_ENCRYPT,     "Encrypted"    },
  { MSG_COMPRESS,    "Compressed"   },
  { MSG_ESCAPED,     "Escaped"      },
  { MSG_FPU,         "ForcePickup"  },
  { MSG_TYPELOCAL,   "TypeLocal"    },
  { MSG_TYPEECHO,    "TypeEcho"     },
  { MSG_TYPENET,     "TypeNet"      },
  { MSG_NODISP,      "NoDisp"       },
  { MSG_LOCKED,      "Locked"       },
  { MSG_DELETED,     "Deleted"      },
  { 0,               NULL           } };

#define WRAP_WIDTH 72
#define LINE_WIDTH 79
#define MAX_WIDTH 997

void copyline(uchar *dest,uchar *src,long len)
{
   int d,c;

   d=0;

   for(c=0;c<len;c++)
     if(src[c] != 10) dest[d++]=src[c];

   dest[d]=0;
}

void sendtextblock(struct var *var,uchar *text,struct xlat *xlat)
{
   long c,d,textpos,lastspace;
   uchar buf[1000],buf2[1000],*xlatres;
   bool wrapped;

   textpos=0;

   while(text[textpos]!=0 && !var->disconnect && !get_server_quit())
   {
      lastspace=0;
      c=0;
      wrapped=FALSE;

      /* Find last space before WRAP_WIDTH */

      while(c<=WRAP_WIDTH && text[textpos+c]!=0 && text[textpos+c]!=13)
      {
         if(text[textpos+c]==32) lastspace=c;
         c++;
      }

      /* It might not be necessary to wrap after all if we find EOL before LINE_WIDTH */

      if(text[textpos+c]!=13 && text[textpos+c]!=0)
      {
         d=c+1;

         while(text[textpos+d]!=0 && text[textpos+d]!=13 && d<LINE_WIDTH)
            d++;

         if(text[textpos+d] == 13 || text[textpos+d] == 0)
            c=d;
      }

      if(text[textpos+c] == 13 || text[textpos+c] == 0)
      {
         /* EOL found */

         copyline(buf,&text[textpos],c);
         if(text[textpos+c]==13) c++;
         textpos+=c;
      }
      else if(lastspace)
      {
         /* Wrap */

         copyline(buf,&text[textpos],lastspace);
         textpos+=lastspace+1;
         wrapped=TRUE;
      }
      else
      {
         /* Just one looong word */

         while(text[textpos+c] != 0 && text[textpos+c] != 13 && text[textpos+c] != 32 && c<MAX_WIDTH)
            c++;

         
         copyline(buf,&text[textpos],c);
         
         if(text[textpos+c] == 32)
            wrapped=TRUE;
         
         if(text[textpos+c] == 32 || text[textpos+c] == 13) 
            c++;
         
         textpos+=c;
      }

      /* Code for format=flowed */

      if(var->opt_flowed && strcmp(buf,"-- ")!=0)
      {
         if(wrapped) strcat(buf," "); /* For format=flowed */
         else        strip(buf);
         
         if(buf[0] == ' ' || strncmp(buf,"From ",5)==0)
         {
            strcpy(buf2,buf);
            strcpy(buf," ");
            strcat(buf,buf2);
         }
      }

      /* End format=flowed */

      if(stricmp(buf,".")==0) /* "." means end of message in NNTP */
         strcpy(buf,"..");

      strcat(buf,CRLF);

      if(xlat && xlat->xlattab)
      {
         if((xlatres=xlatstr(buf,xlat->xlattab)))
         {
            socksendtext(var,xlatres);
            free(xlatres);
         }
      }
      else
      {
         socksendtext(var,buf);
      }   
   }
}

void stripreplyaddr(uchar *str)
{
   /* to take care of "full name" <name@domain> formar */
   
   uchar *ch;
   
   if((ch=strchr(str,'<')))
   {
      strcpy(str,ch+1);

      if((ch=strchr(str,'>')))
         *ch=0;
    }
}

void command_abhs(struct var *var,uchar *cmd)
{
   uchar *article;
   ulong articlenum;
   struct group *group;
   ulong min,max,num,c,d;
   uchar datebuf[50];
   uchar fromaddr[100],toaddr[100],replyaddr[100];
   uchar fromname[100],toname[100],subject[100];
   uchar chrs[20],codepage[20],encoding[20],format[20],timezone[20];
   uchar buf[250];
   uchar *at,*pc;
   uchar *text;
   struct xlat *xlat;
   uchar *xlatres;
   s_JamBaseHeader baseheader;
   s_JamSubPacket* subpack;
   s_JamMsgHeader header;
   s_JamSubfield* field;
   int res;
   ulong count;

   article=parseinput(var);

   if(!article)
   {
      if(!var->currentgroup)
      {
         socksendtext(var,"412 No newsgroup selected" CRLF);
         return;
      }

      if(!var->currentarticle)
      {
         socksendtext(var,"420 No current article has been selected" CRLF);
         return;
      }

      articlenum=var->currentarticle;
      group=var->currentgroup;
   }
   else if(article[0] == '<' && article[strlen(article)-1] == '>')
   {
      strcpy(article,&article[1]);
      article[strlen(article)-1]=0;

      at=strchr(article,'@');
      pc=strchr(article,'$');

      if(!at || !pc)
      {
         socksendtext(var,"430 No such article found" CRLF);
         return;
      }

      *at=0;
      *pc=0;

      at++;
      pc++;

      if(strcmp(at,"JamNNTPd") != 0)
      {
         socksendtext(var,"430 No such article found" CRLF);
         return;
      }

      for(group=var->firstgroup;group;group=group->next)
         if(matchgroup(var->readgroups,group->group) && stricmp(pc,group->tagname) == 0) break;

      if(!group)
      {
         socksendtext(var,"430 No such article found" CRLF);
         return;
      }

      articlenum=atol(article);

      jamgetminmaxnum(var,group,&min,&max,&num);

      if(articlenum < min || articlenum > max)
      {
         socksendtext(var,"430 No such article found" CRLF);
         return;
      }
   }
   else if(atol(article) > 0)
   {
      if(!var->currentgroup)
      {
         socksendtext(var,"412 No newsgroup selected" CRLF);
         return;
      }
      
      articlenum=atol(article);
      group=var->currentgroup;

      if(!jamgetminmaxnum(var,var->currentgroup,&min,&max,&num))
      {
         socksendtext(var,"503 Local error: Could not get size of messagebase" CRLF);
         return;
      }

      jamgetminmaxnum(var,group,&min,&max,&num);

      if(articlenum < min || articlenum > max)
      {
         socksendtext(var,"423 No such article number in this group" CRLF);
         return;
      }

      var->currentarticle = articlenum;
   }
   else
   {
      socksendtext(var,"501 Invalid article number specified" CRLF);
      return;
   }

   if(stricmp(cmd,"STAT") == 0)
   {
      sockprintf(var,"223 %lu <%lu$%s@JamNNTPd> Article retrieved" CRLF,
         articlenum,articlenum,group->tagname);

      return;
   }

   if(!jamopenarea(var,group))
   {
      socksendtext(var,"503 Local error: Could not open messagebase" CRLF);
      return;
   }

   if(JAM_ReadMBHeader(var->openmb,&baseheader))
   {
      os_logwrite("(%s) Could not read messagebase header of \"%s\"",var->clientid,var->opengroup->jampath);
      socksendtext(var,"503 Local error: Could not read messagebase header" CRLF);
      return;
   }

   res=JAM_ReadMsgHeader(var->openmb,articlenum-baseheader.BaseMsgNum,&header,&subpack);

   if(res != 0 && res != JAM_NO_MESSAGE)
   {
      os_logwrite("(%s) Could not read message %lu in \"%s\"",var->clientid,articlenum,var->opengroup->jampath);
      socksendtext(var,"503 Local error: Could not read message header" CRLF);
      JAM_DelSubPacket(subpack);
      return;
   }

   if(res == JAM_NO_MESSAGE || (header.Attribute & MSG_DELETED))
   {
      socksendtext(var,"503 Local error: Message has been deleted" CRLF);
      JAM_DelSubPacket(subpack);
      return;
   }

   if(!(text=malloc(header.TxtLen+1)))
   {
      socksendtext(var,"503 Local error: Out of memory" CRLF);
      JAM_DelSubPacket(subpack);
      return;
   }

   if(header.TxtLen)
   {
     res=JAM_ReadMsgText(var->openmb,header.TxtOffset,header.TxtLen,text);

     if(res)
     {
         socksendtext(var,"503 Local error: Could not read message text" CRLF);
         JAM_DelSubPacket(subpack);
         free(text);
         return;
     }
   }

   text[header.TxtLen]=0;

   /* Find charset */

   chrs[0]=0;
   codepage[0]=0;

   count=0;

   while((field=JAM_GetSubfield_R(subpack,&count)))
   {
      if(field->LoID == JAMSFLD_FTSKLUDGE)
      {
         mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));

         if(strnicmp(buf,"CHRS: ",6)==0)
         {
            mystrncpy(chrs,&buf[6],20);
            if(strchr(chrs,' ')) *strchr(chrs,' ')=0;
            strip(chrs);
         }

         if(strnicmp(buf,"CHARSET: ",9)==0)
         {
            mystrncpy(chrs,&buf[9],20);
            strip(chrs);
         }

         if(strnicmp(buf,"CODEPAGE: ",10)==0)
         {
            mystrncpy(codepage,&buf[10],20);
            strip(codepage);
         }
      }
   }

   xlat=findreadxlat(var,group,chrs,codepage,NULL);

   if(xlat) strcpy(chrs,xlat->tochrs);
   else     strcpy(chrs,"unknown-8bit");

   if(stricmp(cmd,"ARTICLE")==0)
      sockprintf(var,"220 %ld <%ld$%s@JamNNTPd> Article retrieved - Head and body follow" CRLF,articlenum,articlenum,group->tagname);

   if(stricmp(cmd,"HEAD")==0)
      sockprintf(var,"221 %ld <%ld$%s@JamNNTPd> Article retrieved - Head follows" CRLF,articlenum,articlenum,group->tagname);

   if(stricmp(cmd,"BODY")==0)
      sockprintf(var,"222 %ld <%ld$%s@JamNNTPd> Article retrieved - Body follows" CRLF,articlenum,articlenum,group->tagname);

   if(stricmp(cmd,"ARTICLE") == 0 || stricmp(cmd,"HEAD") == 0)
   {
      count=0;

      fromname[0]=0;
      fromaddr[0]=0;
      toname[0]=0;
      toaddr[0]=0;
      subject[0]=0;
      replyaddr[0]=0;
      timezone[0]=0;

      while((field=JAM_GetSubfield_R(subpack,&count)))
      {
         switch(field->LoID)
         {
            case JAMSFLD_OADDRESS:
               mystrncpy(fromaddr,field->Buffer,min(field->DatLen+1,100));
               break;

            case JAMSFLD_SENDERNAME:
               mystrncpy(fromname,field->Buffer,min(field->DatLen+1,100));
               break;

            case JAMSFLD_DADDRESS:
               mystrncpy(toaddr,field->Buffer,min(field->DatLen+1,100));
               break;

            case JAMSFLD_RECVRNAME:
               mystrncpy(toname,field->Buffer,min(field->DatLen+1,100));
               break;

            case JAMSFLD_SUBJECT:
               mystrncpy(subject,field->Buffer,min(field->DatLen+1,100));
               break;

            case JAMSFLD_TZUTCINFO:
               mystrncpy(timezone,field->Buffer,min(field->DatLen+1,20));
               break;

            case JAMSFLD_FTSKLUDGE:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,100));
            
               if(strnicmp(buf,"REPLYADDR ",10)==0)
                  mystrncpy(replyaddr,&buf[10],100);

               if(strnicmp(buf,"REPLYADDR: ",11)==0)
                  mystrncpy(replyaddr,&buf[11],100);

               if(strnicmp(buf,"TZUTC: ",7)==0)
                  mystrncpy(timezone,&buf[7],20);

               if(strnicmp(buf,"TZUTCINFO: ",11)==0)
                  mystrncpy(timezone,&buf[11],20);

               break;
         }
      }

      stripreplyaddr(replyaddr);
      
      if(fromaddr[0] == 0) strcpy(fromaddr,"unknown");
      if(fromname[0] == 0) strcpy(fromname,"unknown");
      if(toname[0] == 0)   strcpy(toname,"(none)");
      
      if(xlat && xlat->xlattab)
      {
         if((xlatres=xlatstr(fromname,xlat->xlattab)))
         {
            mystrncpy(fromname,xlatres,100);
            free(xlatres);
         }

         if((xlatres=xlatstr(toname,xlat->xlattab)))
         {
            mystrncpy(toname,xlatres,100);
            free(xlatres);
         }

         if((xlatres=xlatstr(subject,xlat->xlattab)))
         {
            mystrncpy(subject,xlatres,100);
            free(xlatres);
         }
      }

      makedate(header.DateWritten,datebuf,timezone);

      sockprintf(var,"Path: JamNNTPd!not-for-mail" CRLF);

      if(var->opt_showto) sprintf(buf,"%s -> %s",fromname,toname);
      else                strcpy(buf,fromname);

      if(replyaddr[0]) mimesendheaderline(var,"From",buf,chrs,replyaddr,cfg_noencode);
      else             mimesendheaderline(var,"From",buf,chrs,fromaddr,cfg_noencode);

      mimesendheaderline(var,"X-Comment-To",toname,chrs,NULL,cfg_noencode);
      sockprintf(var,"Newsgroups: %s" CRLF,group->tagname);
      mimesendheaderline(var,"Subject",subject,chrs,NULL,cfg_noencode);

      sockprintf(var,"Date: %s" CRLF,datebuf);
      sockprintf(var,"Message-ID: <%ld$%s@JamNNTPd>" CRLF,articlenum,group->tagname);

      if(header.ReplyTo)
         sockprintf(var,"References: <%ld$%s@JamNNTPd>" CRLF,header.ReplyTo,group->tagname);

      sockprintf(var,"X-JAM-From: %s <%s>" CRLF,fromname,fromaddr);

      if(toname[0])
      {
         if(toaddr[0])
            sockprintf(var,"X-JAM-To: %s <%s>" CRLF,toname,toaddr);

         else
            sockprintf(var,"X-JAM-To: %s" CRLF,toname);
      }

      count=0;

      while((field=JAM_GetSubfield_R(subpack,&count)))
      {
         switch(field->LoID)
         {
            case JAMSFLD_MSGID:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-MSGID: %s" CRLF,buf);
               break;

            case JAMSFLD_REPLYID:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-REPLYID: %s" CRLF,buf);
               break;

            case JAMSFLD_PID:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-PID: %s" CRLF,buf);
               break;

            case JAMSFLD_FLAGS:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-FLAGS: %s" CRLF,buf);
               break;

            case JAMSFLD_TRACE:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-TRACE: %s" CRLF,buf);
               break;

            case JAMSFLD_TZUTCINFO:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-TZUTCINFO: %s" CRLF,buf);
               break;

            case JAMSFLD_SEENBY2D:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-SEENBY2D: %s" CRLF,buf);
               break;

            case JAMSFLD_PATH2D:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-PATH2D: %s" CRLF,buf);
               break;

            case JAMSFLD_FTSKLUDGE:
               mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));
               sockprintf(var,"X-JAM-FTSKLUDGE: %s" CRLF,buf);
               break;
         }
      }

      if(header.Attribute)
      {
         int c;

         strcpy(buf,"X-JAM-Attributes:");

         for(c=0;attributenames[c].name;c++)
            if(header.Attribute & attributenames[c].attr)
            {
               strcat(buf," ");
               strcat(buf,attributenames[c].name);
            }

         strcat(buf,CRLF);
         socksendtext(var,buf);
      }

      /* MIME headers */

      socksendtext(var,"MIME-Version: 1.0" CRLF);

      if(count8bit(text))
      {
         strcpy(encoding,"8bit");
      }
      else
      {
         strcpy(encoding,"7bit");
         strcpy(chrs,"us-ascii");
      }

      if(var->opt_flowed)
         strcpy(format,"flowed");

      else
         strcpy(format,"fixed");

      sockprintf(var,"Content-Type: text/plain; charset=%s; format=%s" CRLF,chrs,format);
      sockprintf(var,"Content-Transfer-Encoding: %s" CRLF,encoding);
   }

   if(stricmp(cmd,"ARTICLE") == 0)
      socksendtext(var,CRLF);

   if(stricmp(cmd,"ARTICLE") == 0 || stricmp(cmd,"BODY") == 0)
   {
      if(header.TxtLen)
      {
         if(!(xlat && xlat->keepsoftcr))
         {
            d=0;
            
            for(c=0;text[c];c++)
               if(text[c] != 0x8d) text[d++]=text[c];
            
            text[d]=0;
         }
         
         sendtextblock(var,text,xlat);
      }     
   }
   
   socksendtext(var,"." CRLF);

   JAM_DelSubPacket(subpack);
   free(text);
}

void command_xover(struct var *var)
{
   uchar *article,*dash;
   ulong min,max,num;
   ulong first,last,c;
   uchar msgid[150],reply[150],buf[250],chrs[20],codepage[20],datebuf[50],timezone[20];
   uchar fromname[100],toname[100],fromaddr[100],subject[100],replyaddr[100];
   uchar mimefrom[1000],mimesubj[1000],xoverres[2500];
   struct xlat *xlat;
   uchar *xlatres;
   s_JamBaseHeader baseheader;
   s_JamSubPacket* subpack;
   s_JamMsgHeader header;
   s_JamSubfield* field;
   int res;
   ulong count;

   if(!var->currentgroup)
   {
      socksendtext(var,"412 No newsgroup selected" CRLF);
      return;
   }

   jamgetminmaxnum(var,var->currentgroup,&min,&max,&num);

   article=parseinput(var);

   if(!article)
   {
      if(!var->currentarticle)
      {
         socksendtext(var,"420 No current article has been selected" CRLF);
         return;
      }

      first=var->currentarticle;
      last=var->currentarticle;
   }
   else
   {
      dash=strchr(article,'-');

      if(dash)
      {
         *dash=0;
         dash++;

         first=atol(article);

         if(dash[0] == 0)
            last=max;

         else
            last=atol(dash);
      }
      else
      {
         first=atol(article);
         last=atol(article);
      }
   }

   if(first < min) first=min;
   if(last > max) last=max;

   if(first > last || num == 0)
   {
      socksendtext(var,"420 No articles found in this range" CRLF);
      return;
   }

   if(!jamopenarea(var,var->currentgroup))
   {
      socksendtext(var,"503 Local error: Could not open messagebase" CRLF);
      return;
   }

   if(JAM_ReadMBHeader(var->openmb,&baseheader))
   {
      os_logwrite("(%s) Could not read messagebase header of \"%s\"",var->clientid,var->opengroup->jampath);
      socksendtext(var,"503 Local error: Could not read messagebase header" CRLF);
      return;
   }

   socksendtext(var,"224 Overview information follows" CRLF);

   for(c=first;c<=last && !var->disconnect && !get_server_quit();c++)
   {
      res=JAM_ReadMsgHeader(var->openmb,c-baseheader.BaseMsgNum,&header,&subpack);

      if(res == 0)
      {
         if(!(header.Attribute & MSG_DELETED))
         {
            count=0;

            fromname[0]=0;
            fromaddr[0]=0;
            subject[0]=0;
            toname[0]=0;
            chrs[0]=0;
            codepage[0]=0;
            replyaddr[0]=0;
            timezone[0]=0;
            
            while((field=JAM_GetSubfield_R(subpack,&count)))
            {
               switch(field->LoID)
               {
                  case JAMSFLD_OADDRESS:
                     mystrncpy(fromaddr,field->Buffer,min(field->DatLen+1,100));
                     break;

                  case JAMSFLD_SENDERNAME:
                     mystrncpy(fromname,field->Buffer,min(field->DatLen+1,100));
                     break;

                  case JAMSFLD_RECVRNAME:
                     mystrncpy(toname,field->Buffer,min(field->DatLen+1,100));
                     break;

                  case JAMSFLD_SUBJECT:
                     mystrncpy(subject,field->Buffer,min(field->DatLen+1,100));
                     break;

                  case JAMSFLD_TZUTCINFO:
                     mystrncpy(timezone,field->Buffer,min(field->DatLen+1,20));
                     break;

                  case JAMSFLD_FTSKLUDGE:
                     mystrncpy(buf,field->Buffer,min(field->DatLen+1,100));

                     if(strnicmp(buf,"CHRS: ",6)==0)
                     {
                        mystrncpy(chrs,&buf[6],20);
                        if(strchr(chrs,' ')) *strchr(chrs,' ')=0;
                        strip(chrs);
                     }

                     if(strnicmp(buf,"CHARSET: ",9)==0)
                        mystrncpy(chrs,&buf[9],20);

                     if(strnicmp(buf,"CODEPAGE: ",10)==0)
                        mystrncpy(codepage,&buf[10],20);

                     if(strnicmp(buf,"REPLYADDR ",10)==0)
                        mystrncpy(replyaddr,&buf[10],100);

                     if(strnicmp(buf,"REPLYADDR: ",11)==0)
                        mystrncpy(replyaddr,&buf[11],100);

                     if(strnicmp(buf,"TZUTC: ",7)==0)
                        mystrncpy(timezone,&buf[7],20);

                     if(strnicmp(buf,"TZUTCINFO: ",11)==0)
                        mystrncpy(timezone,&buf[11],20);
               }
            }

            stripreplyaddr(replyaddr);
            
            if(fromaddr[0] == 0) strcpy(fromaddr,"unknown");
            if(fromname[0] == 0) strcpy(fromname,"unknown");
            if(toname[0] == 0)   strcpy(toname,"(none)");

            xlat=findreadxlat(var,var->currentgroup,chrs,codepage,NULL);

            if(xlat) strcpy(chrs,xlat->tochrs);
            else     strcpy(chrs,"unknown-8bit");

            if(xlat && xlat->xlattab)
            {
               if((xlatres=xlatstr(fromname,xlat->xlattab)))
               {
                  mystrncpy(fromname,xlatres,100);
                  free(xlatres);
               }

               if((xlatres=xlatstr(toname,xlat->xlattab)))
               {
                  mystrncpy(toname,xlatres,100);
                  free(xlatres);
               }

               if((xlatres=xlatstr(subject,xlat->xlattab)))
               {
                  mystrncpy(subject,xlatres,100);
                  free(xlatres);
               }
            }

            makedate(header.DateWritten,datebuf,timezone);

            sprintf(msgid,"<%ld$%s@JamNNTPd>",c,var->currentgroup->tagname);

            reply[0]=0;

            if(header.ReplyTo)
               sprintf(reply,"<%ld$%s@JamNNTPd>",header.ReplyTo,var->currentgroup->tagname);

            if(var->opt_showto)
            {
               sprintf(buf,"%s -> %s",fromname,toname);
               mystrncpy(fromname,buf,100);
            }

            if(replyaddr[0]) mimemakeheaderline(mimefrom,1000,"From",fromname,chrs,replyaddr,cfg_noencode);
            else             mimemakeheaderline(mimefrom,1000,"From",fromname,chrs,fromaddr,cfg_noencode);

            mimemakeheaderline(mimesubj,1000,"Subject",subject,chrs,NULL,cfg_noencode);

            strcpy(mimefrom,&mimefrom[6]);
            strcpy(mimesubj,&mimesubj[9]);

            stripctrl(mimesubj);
            stripctrl(mimefrom);

            sprintf(xoverres,"%ld\t%s\t%s\t%s\t%s\t%s\t\t" CRLF,
                  c,mimesubj,mimefrom,datebuf,msgid,reply);

            socksendtext(var,xoverres);
         }

         JAM_DelSubPacket(subpack);
      }
   }

   socksendtext(var,"." CRLF);
}

#define POST_MAXSIZE 20000

bool getcontenttypepart(uchar *line,ulong *pos,uchar *dest,ulong destlen)
{
   bool quote;
   ulong c,d;

   quote=FALSE;
   c=*pos;
   d=0;

   /* Skip initial whitespace */

   while(isspace(line[c]))
      c++;

   /* Check if there is anything to copy */

   if(line[c] == 0)
   {
      *pos=c;
      return(FALSE);
   }

   /* Copy until ; or 0. Ignore unquoted whitespace */

   for(;;)
   {
      if(line[c] == '"')
      {
         if(quote) quote=FALSE;
         else      quote=TRUE;
      }
      else if(line[c] == 0 || (line[c] == ';' && !quote))
      {
         if(line[c] != 0) c++;
         *pos=c;
         dest[d]=0;
         return(TRUE);
      }
      else if(quote || !isspace(line[c]))
      {
         if(d < destlen-1)
            dest[d++]=line[c];
      }

      c++;
   }
}

void unmimecpy(uchar *dest,uchar *src,ulong destlen,uchar *chrs,uchar *chrs2,ulong chrslen)
{
   unmime(src,chrs,chrs2,chrslen);
   mystrncpy(dest,src,destlen);
}

void unbackslashquote(uchar *text)
{
   int c,d;

   d=0;

   for(c=0;text[c];c++)
   {
      if(text[c] == '\\' && text[c+1] != 0)
         c++;

      text[d++]=text[c];
   }

   text[d]=0;
}

void addjamfield(s_JamSubPacket *SubPacket_PS,ulong fieldnum,uchar *fielddata)
{
   s_JamSubfield	Subfield_S;

   Subfield_S.LoID   = fieldnum;
   Subfield_S.HiID   = 0;
   Subfield_S.DatLen = strlen(fielddata);
   Subfield_S.Buffer = fielddata;

   JAM_PutSubfield( SubPacket_PS, &Subfield_S);
}

void getparentmsgidfromnum(struct var *var,uchar *article,uchar *groupname,uchar *msgid,uchar *from,ulong *oldnum,struct xlat *postxlat)
{
   uchar *at,*pc;
   struct group *group;
   ulong articlenum;
   s_JamBaseHeader baseheader;
   s_JamSubPacket* subpack;
   s_JamMsgHeader header;
   s_JamSubfield* field;
   int res;
   ulong count;
   uchar buf[100],chrs[20],codepage[20];
   struct xlat *xlat;
   uchar *xlatres;
      
   msgid[0]=0;
   from[0]=0;
   *oldnum=0;

   if(article[0] != '<' || article[strlen(article)-1] != '>')
      return;

   strcpy(article,&article[1]);
   article[strlen(article)-1]=0;

   at=strchr(article,'@');
   pc=strchr(article,'$');

   if(!at || !pc)
      return;

   *at=0;
   *pc=0;

   at++;
   pc++;

   if(strcmp(at,"JamNNTPd") != 0)
      return;

   if(stricmp(pc,groupname) == 0)
      *oldnum=atol(article);

   for(group=var->firstgroup;group;group=group->next)
      if(matchgroup(var->readgroups,group->group) && stricmp(pc,group->tagname) == 0) break;

   if(!group)
      return;

   articlenum=atol(article);

   if(!jamopenarea(var,group))
      return;

   if(JAM_ReadMBHeader(var->openmb,&baseheader))
   {
      os_logwrite("(%s) Could not read messagebase header of \"%s\"",var->clientid,var->opengroup->jampath);
      return;
   }

   res=JAM_ReadMsgHeader(var->openmb,articlenum-baseheader.BaseMsgNum,&header,&subpack);

   if(res != 0 && res != JAM_NO_MESSAGE)
   {
      os_logwrite("(%s) Could not read message %lu in \"%s\"",var->clientid,articlenum,var->opengroup->jampath);
      JAM_DelSubPacket(subpack);
      return;
   }

   if(res == JAM_NO_MESSAGE)
   {
      JAM_DelSubPacket(subpack);
      return;
   }

   count=0;

   while((field=JAM_GetSubfield_R(subpack,&count)))
   {
      if(field->LoID == JAMSFLD_MSGID)
         mystrncpy(msgid,field->Buffer,min(field->DatLen+1,100));

      if(field->LoID == JAMSFLD_SENDERNAME)
         mystrncpy(from,field->Buffer,min(field->DatLen+1,100));

      if(field->LoID == JAMSFLD_FTSKLUDGE)
      {
         mystrncpy(buf,field->Buffer,min(field->DatLen+1,100));

         if(strnicmp(buf,"CHRS: ",6)==0)
         {
            mystrncpy(chrs,&buf[6],20);
            if(strchr(chrs,' ')) *strchr(chrs,' ')=0;
            strip(chrs);
         }

         if(strnicmp(buf,"CHARSET: ",9)==0)
            mystrncpy(chrs,&buf[9],20);

         if(strnicmp(buf,"CODEPAGE: ",10)==0)
            mystrncpy(codepage,&buf[10],20);
      }
   }                     

   xlat=findreadxlat(var,group,chrs,codepage,postxlat->fromchrs);
   
   if(xlat && xlat->xlattab)
   {
      if((xlatres=xlatstr(from,xlat->xlattab)))
      {
         mystrncpy(from,xlatres,100);
         free(xlatres);
      }
   
      if(postxlat->xlattab)
      {
         if((xlatres=xlatstr(from,postxlat->xlattab)))
         {
            mystrncpy(from,xlatres,100);
            free(xlatres);
         }
      }
   }
   
   JAM_DelSubPacket(subpack);
   return;
}

/* line should have some extra room. line may grow by one or two characters */
void tidyquote(char *line)
{
   int lastquote,numquotes,c;
   char *input,*initials;
   
   if(!(input=strdup(line)))
      return;

   strip(input);
   
   numquotes=0;
   lastquote=0;      
         
   for(c=0;input[c]!=0;c++)
   {
      if(input[c] == '>')
      {
         lastquote=c;
         numquotes++;
      }
      else if(input[c] == '<')
      {
         break;
      }
      else if(input[c] != ' ' && input[c+1] == ' ')
      {
         break;
      }
   }
               
   if(numquotes)
   {
      initials="";
                  
      /* Find end of initials */
      
      c=lastquote;
      
      while(c > 0 && input[c] == '>')
         c--;
         
      if(input[c] != '>')
      {
         /* Initials found */
         
         input[c+1]=0;
         
         while(c > 0 && input[c] != ' ' && input[c] != '>')
            c--;
            
         if(input[c] == ' ' || input[c] == '>') initials=&input[c+1];         
         else                                   initials=&input[c];
      }

      /* Recreate line */
      
      if(input[lastquote+1] == 0)
      {
         strcpy(line,"\x0d");
      }
      else            
      {
         strcpy(line," ");
         strcat(line,initials);
      
         for(c=0;c<numquotes;c++)
            strcat(line,">");
         
         strcat(line," ");
      
         if(input[lastquote+1] == ' ') strcat(line,&input[lastquote+2]);
         else                          strcat(line,&input[lastquote+1]);
      
         strcat(line,"\x0d");
      }
   }         

   free(input);
}

uchar *smartquote(uchar *oldtext,ulong maxlen,uchar *fromname)
{
   uchar *newtext,line[300],mark[100];
   int c,d,linebegins;

   if(!(newtext=malloc(maxlen)))
      return(NULL);

   d=0;

   for(c=0;fromname[c];c++)
      if(c==0 || (c!=0 && fromname[c-1]==32)) mark[d++]=fromname[c];

   mark[d]=0;

   c=0;
   d=0;

   while(oldtext[c])
   {
      linebegins=c;

      while(oldtext[c] != 13 && oldtext[c] != 0)
         c++;

      if(oldtext[c] == 13)
         c++;

      if(oldtext[linebegins] == '>' && c-linebegins < 200)
      {
         strcpy(line,mark);
         strncat(line,&oldtext[linebegins],c-linebegins);
         tidyquote(line);

         if(strlen(line)+d+1 > maxlen)
         {
            /* We ran out of room */
            free(newtext);
            return(NULL);
         }

         strcpy(&newtext[d],line);
         d+=strlen(line);
      }
      else
      {
         if(c-linebegins+d+1 > maxlen)
         {
            /* We ran out of room */
            free(newtext);
            return(NULL);
         }

         strncpy(&newtext[d],&oldtext[linebegins],c-linebegins);
         d+=c-linebegins;
      }
   }

   newtext[d]=0;

   return(newtext);
}

void setreply(struct var *var,ulong parentmsg,ulong newmsg)
{
   s_JamBaseHeader baseheader;
   s_JamMsgHeader header;
   int res;
   ulong nextreply,msg;

   if(JAM_ReadMBHeader(var->openmb,&baseheader))
   {
      os_logwrite("(%s) Could not read messagebase header of \"%s\"",var->clientid,var->opengroup->jampath);
      return;
   }

   /* Get parent message */

   res=JAM_ReadMsgHeader(var->openmb,parentmsg-baseheader.BaseMsgNum,&header,NULL);

   if(res != 0)
      return;

   if(header.Reply1st == 0)
   {
      /* This is the first reply. Set Reply1st and update header */

      header.Reply1st=newmsg;
      JAM_ChangeMsgHeader(var->openmb,parentmsg-baseheader.BaseMsgNum,&header);
      return;
   }

   /* There were other replies. Find the last. */

   nextreply=header.Reply1st;
   msg=0;

   while(nextreply)
   {
      res=JAM_ReadMsgHeader(var->openmb,nextreply-baseheader.BaseMsgNum,&header,NULL);

      if(res != 0)
         return;

      msg=nextreply;
      nextreply=header.ReplyNext;
   }

   /* Found last reply. Set ReplyNext and update header */

   header.ReplyNext=newmsg;
   JAM_ChangeMsgHeader(var->openmb,msg-baseheader.BaseMsgNum,&header);
}

void command_post(struct var *var)
{
   uchar *text,*newtext,*xlatres,line[1000],buf[100];
   ulong allocsize,textlen,textpos,getctpos,c,d,parentmsg;
   bool finished,toobig;
   uchar from[100],fromaddr[100],toname[100],subject[100],organization[100],newsgroup[100];
   uchar contenttype[100],contenttransferencoding[100],reference[100],newsreader[100];
   uchar msgid[100],replyid[100],replyto[100],chrs[20],chrs2[20],codepage[20],timezone[13],control[100];
   struct group *g;
   struct xlat *xlat;
   s_JamSubPacket*	SubPacket_PS;
   s_JamMsgHeader	Header_S;
   time_t t1,t2;
   struct tm *tp;
   int res,timeofs,timesign,tr;
   bool flowed;
   FILE *fp;
   
   allocsize=POST_MAXSIZE+500; /* Some extra room for tearline and origin */

   if(!(text=malloc(allocsize)))
   {
      socksendtext(var,"503 Out of memory" CRLF);
      return;
   }

   socksendtext(var,"340 Send article to be posted. End with <CR-LF>.<CR-LF>" CRLF);

   finished=FALSE;
   toobig=FALSE;

   textpos=0;

   while(!finished && !var->disconnect && !get_server_quit() && sockreadline(var,line,1000))
   {
      if(line[0] && cfg_debug)
         printf("(%s) < %s",var->clientid,line);

      if(stricmp(line,"." CRLF) == 0)
      {
         finished=TRUE;
      }
      else
      {
         if(textpos + strlen(line) > POST_MAXSIZE-1)
         {
            toobig=TRUE;
         }
         else
         {
            strcpy(&text[textpos],line);
            textpos+=strlen(line);
         }
      }
   }

   text[textpos]=0;
   
   if(!get_server_quit)
   {
      free(text);
      return;
   }

   if(toobig)
   {
      sockprintf(var,"441 Posting failed (message too long, maximum size %ld bytes" CRLF,POST_MAXSIZE);
      os_logwrite("(%s) POST failed (message too long, maximum size %ld bytes)",var->clientid,POST_MAXSIZE);
      free(text);
      return;
   }

   from[0]=0;
   fromaddr[0]=0;
   newsgroup[0]=0;
   subject[0]=0;
   replyto[0]=0;
   contenttype[0]=0;
   chrs[0]=0;
   chrs2[0]=0;
   contenttransferencoding[0]=0;
   reference[0]=0;
   organization[0]=0;
   newsreader[0]=0;
   control[0]=0;
   flowed=FALSE;

   textpos=0;
   textlen=strlen(text);

   finished=FALSE;

   while(text[textpos] != 0 && !finished)
   {
      c=0;

      for(;;)
      {
         if(text[textpos] == 0)
         {
            break;
         }
         else if(c>0 && text[textpos-1] == 13 && text[textpos] == 10)
         {
            if(c>1 && (text[textpos+1] == ' ' || text[textpos+1] == '\t'))
            {
               /* Multi-line header */

               while(text[textpos+1] == ' ' || text[textpos+1] == '\t')
                  textpos++;

               line[c-1]=' ';
            }
            else
            {
               line[c-1]=0;
               textpos++;
               break;
            }
         }
         else
         {
            if(c<999)
               line[c++]=text[textpos];
         }

         textpos++;
      }

      line[c]=0;
      strip(line);

      if(strnicmp(line,"From: ",6)==0)
      {
         if(line[strlen(line)-1] == '>' && strchr(line,'<'))
         {
            /* From: Mark Horton <mark@cbosgd.ATT.COM> */

            line[strlen(line)-1]=0;
            unmimecpy(fromaddr,strrchr(line,'<')+1,100,chrs,chrs2,20);
            strip(fromaddr);

            *strchr(line,'<')=0;
            unmimecpy(from,&line[6],100,chrs,chrs2,20);
            strip(from);
         }
         else if(line[strlen(line)-1] == ')' && strchr(line,'('))
         {
            /* From: mark@cbosgd.ATT.COM (Mark Horton) */

            line[strlen(line)-1]=0;
            unbackslashquote(strrchr(line,'(')+1); /* Comments should be un-backslash-quoted */
            unmimecpy(from,strrchr(line,'(')+1,100,chrs,chrs2,20);
            strip(from);

            *strrchr(line,'(')=0;
            unmimecpy(fromaddr,&line[6],100,chrs,chrs2,20);
            strip(fromaddr);
         }
         else
         {
            unmimecpy(from,&line[6],100,chrs,chrs2,20);
            unmimecpy(fromaddr,&line[6],100,chrs,chrs2,20);
         }

         if(strlen(from) > 0)
         {
            /* Remove quotes if any */
            
            if(from[0] == '\"' && from[strlen(from)-1]=='\"')
            {
               from[strlen(from)-1]=0;
               strcpy(from,&from[1]);
               unbackslashquote(from); /* Text in "" should be un-backslash-quoted */
            }
         }
      }
      else if(strnicmp(line,"Newsgroups: ",12)==0)
      {
         mystrncpy(newsgroup,&line[12],100);
      }
      else if(strnicmp(line,"Subject: ",9)==0)
      {
         unmimecpy(subject,&line[9],100,chrs,chrs2,20);
      }
      else if(strnicmp(line,"Reply-To: ",10)==0)
      {
         unmimecpy(replyto,&line[10],100,chrs,chrs2,20);
      }
      else if(strnicmp(line,"Content-Type: ",14)==0)
      {
         getctpos=14;

         if(!getcontenttypepart(line,&getctpos,contenttype,100))
            contenttype[0]=0;

         while(getcontenttypepart(line,&getctpos,buf,100))
         {
            if(strnicmp(buf,"charset=",8)==0)
               setcharset(&buf[8],chrs,chrs2,20);

            else if(stricmp(buf,"format=flowed")==0)
               flowed=TRUE;
         }
      }
      else if(strnicmp(line,"Content-Transfer-Encoding: ",27)==0)
      {
         getctpos=27;

         if(!getcontenttypepart(line,&getctpos,contenttransferencoding,100))
            contenttransferencoding[0]=0;
      }
      else if(strnicmp(line,"References: ",12)==0)
      {
         if(strrchr(line,'<'))
            mystrncpy(reference,strrchr(line,'<'),100);
      }
      else if(strnicmp(line,"Organization: ",14)==0)
      {
         unmimecpy(organization,&line[14],100,chrs,chrs2,20);
      }
      else if(strnicmp(line,"X-Newsreader: ",14)==0)
      {
         unmimecpy(newsreader,&line[14],100,chrs,chrs2,20);
      }
      else if(strnicmp(line,"User-Agent: ",12)==0)
      {
         unmimecpy(newsreader,&line[12],100,chrs,chrs2,20);
      }
      else if(strnicmp(line,"Control: ",9)==0)
      {
         mystrncpy(control,&line[9],100);
      }
      else if(line[0] == 0)
      {
         finished=TRUE; /* End of headers */
      }
   }
   
   /* Strip Re: */

   if(!cfg_nostripre && (strncmp(subject,"Re: ",4)==0 || strcmp(subject,"Re:")==0))
      strcpy(subject,&subject[4]);

   /* Truncate strings */

   from[36]=0;
   subject[72]=0;
   organization[70]=0;
   newsreader[75]=0;

   /* Check syntax */

   if(newsgroup[0] == 0)
   {
      sockprintf(var,"441 Posting failed (No valid Newsgroups line found)" CRLF);
      os_logwrite("(%s) POST failed (No valid Newsgroups line found)",var->clientid);
      free(text);
      return;
   }

   if(from[0] == 0 || fromaddr[0] == 0)
   {
      sockprintf(var,"441 Posting failed (No valid From line found)" CRLF);
      os_logwrite("(%s) POST failed (No valid From line found)",var->clientid);
      free(text);
      return;
   }

   if(strchr(newsgroup,','))
   {
      sockprintf(var,"441 Posting failed (Crossposts are not allowed)" CRLF);
      os_logwrite("(%s) POST failed (Crossposts are not allowed)",var->clientid);
      free(text);
      return;
   }

   if(contenttype[0] && stricmp(contenttype,"text/plain")!=0)
   {
      sockprintf(var,"441 Posting failed (Content-Type \"%s\" not allowed, please use text/plain)" CRLF,contenttype);
      os_logwrite("(%s) POST failed (Content-Type \"%s\" not allowed)",var->clientid,contenttype);
      free(text);
      return;
   }

   if(strnicmp(control,"cancel ",7)==0)
   {
      sockprintf(var,"441 Posting failed (Cancel messages are not supported)" CRLF);
      os_logwrite("(%s) POST failed (Cancel messages are not supported)",var->clientid);
      free(text);
      return;
   }
   
   /* Decode message */
   
   if(stricmp(contenttransferencoding,"quoted-printable")==0)
   {
      decodeqpbody(&text[textpos],&text[textpos]);
   }
   else if(stricmp(contenttransferencoding,"base64")==0)
   {
      decodeb64(&text[textpos],&text[textpos]);
   }
   else if(contenttransferencoding[0] && stricmp(contenttransferencoding,"8bit")!=0 && stricmp(contenttransferencoding,"7bit")!=0)
   {
      sockprintf(var,"441 Posting failed (unknown Content-Transfer-Encoding \"%s\")" CRLF,contenttransferencoding);
      os_logwrite("(%s) POST failed (Content-Transfer-Encoding \"%s\" not allowed)",var->clientid,contenttransferencoding);
      free(text);
      return;
   }
   
   /* Reformat text */

   d=0;

   while(text[textpos])
   {
      c=0;

      for(;;)
      {
         if(text[textpos] == 0)
         {
            break;
         }
         else if(text[textpos] == 13 || c==999)
         {
            textpos++;
            break;
         }
         else
         {
            if(text[textpos]!=10) line[c++]=text[textpos];
            textpos++;
         }
      }

      line[c]=0;

      if(flowed && line[0]!=0 && line[0]!='>' && strncmp(line,"-- ",3)!=0)
      {
         if(line[0] == ' ')
            strcpy(line,&line[1]);

         if(line[strlen(line)-1] == ' ')
         {
            strip(line);
            strcpy(&text[d],line);
            d+=strlen(line);
            text[d++]=' ';
         }
         else
         {
            strip(line);
            strcpy(&text[d],line);
            d+=strlen(line);
            text[d++]=13;
         }
      }
      else
      {
         if(strncmp(line,"-- ",3)!=0)
            strip(line);

         strcpy(&text[d],line);
         d+=strlen(line);
         text[d++]=13;
      }
   }

   /* Reformat CR:s at the end of the text */

   while(d > 0 && text[d-1] == 13) d--;

   if(d > 0 && d <= POST_MAXSIZE-3)
   {
      text[d++]=13;
      text[d++]=13;
   }
   
   text[d]=0;

   /* Check access rights */

   for(g=var->firstgroup;g;g=g->next)
      if(stricmp(newsgroup,g->tagname)==0) break;

   if(!g)
   {
      sockprintf(var,"441 Posting failed (Unknown newsgroup %s)" CRLF,newsgroup);
      os_logwrite("(%s) POST failed (Unknown newsgroup %s)",var->clientid,newsgroup);
      free(text);
      return;
   }

   if(!(matchgroup(var->postgroups,g->group)))
   {
      sockprintf(var,"441 Posting failed (Posting access denied to %s)" CRLF,newsgroup);
      os_logwrite("(%s) POST failed (Posting access denied to %s)",var->clientid,newsgroup);
      free(text);
      return;
   }

   /* Check charset */

   if(chrs2[0])
   {
      sockprintf(var,"441 Posting failed (Message contains multiple charsets, \"%s\" and \"%s\")" CRLF,chrs,chrs2);
      os_logwrite("(%s) POST failed (Message contains multiple charsets, \"%s\" and \"%s\")",var->clientid,chrs,chrs2);
      free(text);
      return;
   }

   if(g->defaultchrs[0] == '!' || var->defaultreadchrs[0] == '!')
   {
      if(g->defaultchrs[0] == '!')
         xlat=findpostxlat(var,chrs,&g->defaultchrs[1]);
   
      else
         xlat=findpostxlat(var,chrs,&var->defaultreadchrs[1]);
   
      if(!xlat)
      {
         sockprintf(var,"441 Posting failed (Unsupported charset \"%s\" for area %s)" CRLF,chrs,g->tagname);
         os_logwrite("(%s) POST failed (Unsupported charset \"%s\" for area %s)",var->clientid,chrs,g->tagname);
         free(text);
         return;
      }     
   }
   else
   {      
      xlat=findpostxlat(var,chrs,NULL);

      if(!xlat)
      {
         sockprintf(var,"441 Posting failed (Unsupported charset \"%s\")" CRLF,chrs);
         os_logwrite("(%s) POST failed (Unsupported charset \"%s\")",var->clientid,chrs);
         free(text);
         return;
      }     
   }
   
   /* Make JAM header */

   JAM_ClearMsgHeader(&Header_S);

   if(!(SubPacket_PS = JAM_NewSubPacket()))
   {
      socksendtext(var,"503 Local error: JAM_NewSubPacket() failed" CRLF);
      free(text);
      return;
   }

   t1=time(NULL);
   tp=gmtime(&t1);
   tp->tm_isdst=-1;
   t2=mktime(tp);

   timeofs=(t1-t2)/60;
   timesign=timeofs < 0 ? -1 : 1;

   sprintf(timezone,"TZUTC: %s%02d%02d",
           (t1 < t2 ? "-" : ""),
           (timesign * timeofs) / 60,
           (timesign * timeofs) % 60);

   Header_S.DateWritten   = time(NULL)-t2+t1;
   Header_S.DateReceived  = time(NULL)-t2+t1;
   Header_S.DateProcessed = time(NULL)-t2+t1;

   /* Set MSGID and REPLY */

   replyid[0]=0;
   msgid[0]=0;
   parentmsg=0;
   toname[0]=0;

   sprintf(msgid,"%s %08lx",g->aka,get_msgid_num());
   addjamfield(SubPacket_PS,JAMSFLD_MSGID,msgid);

   if(reference[0])
   {
      getparentmsgidfromnum(var,reference,g->tagname,replyid,toname,&parentmsg,xlat);

      addjamfield(SubPacket_PS,JAMSFLD_REPLYID,replyid);
      Header_S.ReplyTo  = parentmsg;

      toname[36]=0;

      if(cfg_smartquote)
      {
         if((newtext=smartquote(text,allocsize,toname)))
         {
            free(text);
            text=newtext;
         }
      }
   }
   else
   {
      strcpy(toname,"All");
   }

   Header_S.MsgIdCRC = JAM_Crc32(msgid,strlen(msgid));
   Header_S.ReplyCRC = JAM_Crc32(replyid,strlen(replyid));

   /* Add tearline and origin */

   if(newsreader[0]==0 || cfg_notearline) strcpy(line,"---" CR);
   else                                   sprintf(line,"--- %s" CR,newsreader);

   if(strlen(text) + strlen(line) < allocsize-1)
      strcat(text,line);

   if(cfg_origin) sprintf(line," * Origin: %s (%s)" CR,cfg_origin,g->aka);
   else           sprintf(line," * Origin: %s (%s)" CR,organization,g->aka);

   if(strlen(text) + strlen(line) < allocsize-1)
      strcat(text,line);

   if(var->dispname[0])
      strcpy(from,var->dispname);
   
   if(!var->login && cfg_guestsuffix)
   {
      tr=36-strlen(cfg_guestsuffix)-1;
      if(tr < 0) tr=0;
      
      from[tr]=0;
      strcat(from,cfg_guestsuffix);
   }      
   
   /* Do xlat */

   if(xlat->xlattab)
   {
      if((xlatres=xlatstr(from,xlat->xlattab)))
      {
         mystrncpy(from,xlatres,36);
         free(xlatres);
      }

      if((xlatres=xlatstr(subject,xlat->xlattab)))
      {
         mystrncpy(subject,xlatres,72);
         free(xlatres);
      }

      if((xlatres=xlatstr(text,xlat->xlattab)))
      {
         free(text);
         text=xlatres;
      }
   }
     
   addjamfield(SubPacket_PS,JAMSFLD_SENDERNAME,from);
   addjamfield(SubPacket_PS,JAMSFLD_RECVRNAME,toname);
   addjamfield(SubPacket_PS,JAMSFLD_SUBJECT,subject);
   addjamfield(SubPacket_PS,JAMSFLD_OADDRESS,g->aka);

   if(!cfg_noreplyaddr)
   {
      if(replyto[0]) sprintf(line,"REPLYADDR %s",replyto);
      else           sprintf(line,"REPLYADDR %s",fromaddr);

      addjamfield(SubPacket_PS,JAMSFLD_FTSKLUDGE,line);
   }

   strcpy(line,SERVER_NAME " " SERVER_PIDVERSION);
   addjamfield(SubPacket_PS,JAMSFLD_PID,line);

   if(xlat->tochrs[0] && !g->nochrs)
   {
      setchrscodepage(chrs,codepage,xlat->tochrs);
      
      if(chrs[0])
      {
         sprintf(line,"CHRS: %s 2",chrs);
         addjamfield(SubPacket_PS,JAMSFLD_FTSKLUDGE,line);
      }
      
      if(codepage[0])
      {
         sprintf(line,"CODEPAGE: %s",codepage);
         addjamfield(SubPacket_PS,JAMSFLD_FTSKLUDGE,line);
      }
   }

   if(!cfg_notzutc)
      addjamfield(SubPacket_PS,JAMSFLD_FTSKLUDGE,timezone);

   Header_S.Attribute = MSG_LOCAL | MSG_TYPEECHO;

   /* Write message */

   if(!jamopenarea(var,g))
   {
      socksendtext(var,"503 Local error: Could not open messagebase" CRLF);
      free(text);
      JAM_DelSubPacket(SubPacket_PS);
      return;
   }

   if(JAM_LockMB(var->openmb,10))
   {
      os_logwrite("(%s) Failed to lock JAM messagebase \"%s\"",var->clientid,g->jampath);
      socksendtext(var,"503 Local error: Failed to lock messagebase" CRLF);
      free(text);
      JAM_DelSubPacket(SubPacket_PS);
      return;
   }
   
   res=JAM_AddMessage(var->openmb,&Header_S,SubPacket_PS,text,strlen(text));
   
   if(res)
   {
      socksendtext(var,"503 Local error: Failed to write to messagebase" CRLF);
      os_logwrite("(%s) Failed to write message to JAM messagebase \"%s\"",var->clientid,g->jampath);
   }
   else
   {
      socksendtext(var,"240 Article posted" CRLF);
      os_logwrite("(%s) Posted message to %s (#%lu)",var->clientid,newsgroup,Header_S.MsgNum);
   }

   JAM_DelSubPacket(SubPacket_PS);

   if(parentmsg)
      setreply(var,parentmsg,Header_S.MsgNum);

   JAM_UnlockMB(var->openmb);

   if(cfg_echomailjam)
   {
      if(!(fp=fopen(cfg_echomailjam,"a")))
      {
         os_logwrite("(%s) Failed to open %s",var->clientid,cfg_echomailjam);
      }
      else
      {
         fprintf(fp,"%s %ld\n",g->jampath,Header_S.MsgNum);
         fclose(fp);
      }
   }
   
   
   free(text);
}

void command_authinfo(struct var *var)
{
   uchar *tmp,*opt,*next,*equal;
   bool flowed,showto;

   if(!(tmp=parseinput(var)))
   {
      socksendtext(var,"501 Only AUTHINFO USER or AUTHINFO pass are understood" CRLF);
      return;
   }

   if(stricmp(tmp,"user")!=0 && stricmp(tmp,"pass")!=0)
   {
      socksendtext(var,"501 Only AUTHINFO USER or AUTHINFO pass are understood" CRLF);
      return;
   }

   if(stricmp(tmp,"user")==0)
   {
      if(!(tmp=parseinput(var)))
      {
         socksendtext(var,"482 No user specified for AUTHINFO USER" CRLF);
         return;
      }

      mystrncpy(var->loginname,tmp,100);

      socksendtext(var,"381 Received login name, now send password" CRLF);
      return;
   }

   /* AUTHINFO PASS */

   if(var->loginname[0] == 0)
   {
      socksendtext(var,"482 Use AUTHINFO USER before AUTHINFO pass" CRLF);
      return;
   }

   if(!(tmp=parseinput(var)))
   {
      socksendtext(var,"482 No password specified for AUTHINFO PASS" CRLF);
      return;
   }

   mystrncpy(var->password,tmp,100);

   /* Parse loginname */

   opt=NULL;

   flowed=var->opt_flowed;
   showto=var->opt_showto;

   if(strchr(var->loginname,'/'))
   {
      opt=strchr(var->loginname,'/');
      *opt=0;
      opt++;
   }

   while(opt)
   {
      next=strchr(opt,',');

      if(next)
      {
         *next=0;
         next++;
      }

      equal=strchr(opt,'=');

      if(!equal)
      {
         sockprintf(var,"482 Invalid option format %s, use option=on/off" CRLF,opt);
         return;
      }

      *equal=0;
      equal++;

      if(stricmp(opt,"flowed")==0)
      {
         if(!(setboolonoff(equal,&flowed)))
         {
            sockprintf(var,"482 Unknown setting %s for option %s, use on or off" CRLF,equal,opt);
            return;
         }
      }
      else if(stricmp(opt,"showto")==0)
      {
         if(!(setboolonoff(equal,&showto)))
         {
            sockprintf(var,"482 Unknown setting %s for option %s, use on or off" CRLF,equal,opt);
            return;
         }
      }
      else
      {
         sockprintf(var,"482 Unknown option %s, known options: flowed, showto" CRLF,opt);
         return;
      }

      opt=next;
   }

   if(var->loginname[0])
   {
      if(!(login(var,var->loginname,var->password)))
      {
         socksendtext(var,"481 Authentication rejected" CRLF);
         return;
      }

      socksendtext(var,"281 Authentication accepted" CRLF);
   }
   else
   {
      socksendtext(var,"281 Authentication accepted (options set, no login)"  CRLF);
   }

   var->opt_flowed=flowed;
   var->opt_showto=showto;

   return;
}

void server(SOCKET s)
{
   uchar line[1000],lookup[200],*cmd;
   struct var var;

   struct hostent *hostent;
   struct sockaddr_in fromsa;
   int fromsa_len = sizeof(struct sockaddr_in);

   os_getexclusive();
   server_openconnections++;
   os_stopexclusive();

   var.disconnect=0;

   var.currentgroup=NULL;
   var.currentarticle=0;

   var.openmb=NULL;
   var.opengroup=NULL;

   var.firstgroup=NULL;
   var.firstreadxlat=NULL;
   var.firstpostxlat=NULL;
   var.firstreadalias=NULL;
   var.firstpostalias=NULL;
   var.firstxlattab=NULL;

   var.readgroups[0]=0;
   var.postgroups[0]=0;

   var.loginname[0]=0;
   var.password[0]=0;

   var.opt_flowed=cfg_def_flowed;
   var.opt_showto=cfg_def_showto;

   if(getpeername(s,(struct sockaddr *)&fromsa,&fromsa_len) == SOCKET_ERROR)
   {
      os_showerror("getpeername() failed");

      shutdown(s,2);
      close(s);

      os_getexclusive();
      server_openconnections--;
      os_stopexclusive();

      return;
   }

   if(!(var.sio=allocsockio(s)))
   {
      os_showerror("allocsockio() failed");

      shutdown(s,2);
      close(s);

      os_getexclusive();
      server_openconnections--;
      os_stopexclusive();

      return;
   }

   sprintf(var.clientid,"%s:%u",inet_ntoa(fromsa.sin_addr),ntohs(fromsa.sin_port));

   mystrncpy(lookup,inet_ntoa(fromsa.sin_addr),200);

   if((hostent=gethostbyaddr((char *)&fromsa.sin_addr,sizeof(fromsa.sin_addr),AF_INET)))
      mystrncpy(lookup,hostent->h_name,200);

   os_logwrite("(%s) Connection established to %s",var.clientid,lookup);

   if(!checkallow(&var,inet_ntoa(fromsa.sin_addr)))
   {
      socksendtext(&var,"502 Access denied." CRLF);
      os_logwrite("(%s) Access denied (not in allow list)",var.clientid);

      shutdown(s,2);
      close(s);
      freesockio(var.sio);

      os_getexclusive();
      server_openconnections--;
      os_stopexclusive();

      return;
   }

   if((ulong)get_server_openconnections() > cfg_maxconn)
   {
      os_logwrite("(%s) Access denied (server full)",var.clientid);
      socksendtext(&var,"502 Maximum number of connections reached, please try again later" CRLF);

      shutdown(s,2);
      close(s);
      freesockio(var.sio);

      os_getexclusive();
      server_openconnections--;
      os_stopexclusive();

      return;
   }

   if(!(readgroups(&var)))
   {
      socksendtext(&var,"503 Failed to read group configuration file" CRLF);

      shutdown(s,2);
      close(s);
      freesockio(var.sio);

      os_getexclusive();
      server_openconnections--;
      os_stopexclusive();

      return;
   }


   if(!(readxlat(&var)))
   {
      socksendtext(&var,"503 Failed to read xlat configuration file" CRLF);

      shutdown(s,2);
      close(s);
      freesockio(var.sio);
      freegroups(&var);

      os_getexclusive();
      server_openconnections--;
      os_stopexclusive();

      return;
   }


   socksendtext(&var,"200 Welcome to " SERVER_NAME " " SERVER_VERSION " (posting may or may not be allowed, try your luck)" CRLF);

   while(!var.disconnect && !get_server_quit() && sockreadline(&var,line,1000))
   {
      strip(line);

      var.input=line;
      var.inputpos=0;

      if(line[0] && cfg_debug)
         printf("(%s) < %s\n",var.clientid,line);

      if((cmd=parseinput(&var)))
      {
         if(stricmp(cmd,"ARTICLE")==0)
         {
            command_abhs(&var,cmd);
         }
         else if(stricmp(cmd,"AUTHINFO")==0)
         {
            command_authinfo(&var);
         }
         else if(stricmp(cmd,"BODY")==0)
         {
            command_abhs(&var,cmd);
         }
         else if(stricmp(cmd,"HEAD")==0)
         {
            command_abhs(&var,cmd);
         }
         else if(stricmp(cmd,"STAT")==0)
         {
            command_abhs(&var,cmd);
         }
         else if(stricmp(cmd,"GROUP")==0)
         {
            command_group(&var);
         }
         else if(stricmp(cmd,"HELP")==0)
         {
            socksendtext(&var,"100 Help text follows" CRLF);
            socksendtext(&var,"Recognized commands:" CRLF);
            socksendtext(&var,CRLF);
            socksendtext(&var,"ARTICLE" CRLF);
            socksendtext(&var,"AUTHINFO" CRLF);
            socksendtext(&var,"BODY" CRLF);
            socksendtext(&var,"GROUP" CRLF);
            socksendtext(&var,"HEAD" CRLF);
            socksendtext(&var,"HELP" CRLF);
            socksendtext(&var,"IHAVE (not implemented, messages are always rejected)" CRLF);
            socksendtext(&var,"LAST" CRLF);
            socksendtext(&var,"LIST" CRLF);
            socksendtext(&var,"NEWGROUPS (not implemented, always returns an empty list)" CRLF);
            socksendtext(&var,"NEWNEWS (not implemented, always returns an empty list)" CRLF);
            socksendtext(&var,"NEXT" CRLF);
            socksendtext(&var,"QUIT" CRLF);
            socksendtext(&var,"SLAVE (has no effect)" CRLF);
            socksendtext(&var,"STAT" CRLF);
            socksendtext(&var,"XOVER (partially implemented, byte count and line count are always empty)" CRLF);
            socksendtext(&var,CRLF);
            socksendtext(&var,"JamNNTPd supports most of RFC-977 and also has support for AUTHINFO and" CRLF);
            socksendtext(&var,"limited XOVER support (RFC-2980)" CRLF);
            socksendtext(&var,"." CRLF);
         }
         else if(stricmp(cmd,"IHAVE")==0)
         {
            socksendtext(&var,"435 Article not wanted - do not send it" CRLF);
         }
         else if(stricmp(cmd,"LAST")==0)
         {
            command_last(&var);
         }
         else if(stricmp(cmd,"LIST")==0)
         {
            command_list(&var);
         }
         else if(stricmp(cmd,"NEWGROUPS")==0)
         {
            socksendtext(&var,"231 Warning: NEWGROUPS not implemented, returning empty list" CRLF);
            socksendtext(&var,"." CRLF);
         }
         else if(stricmp(cmd,"NEWNEWS")==0)
         {
            socksendtext(&var,"230 Warning: NEWNEWS not implemented, returning empty list" CRLF);
            socksendtext(&var,"." CRLF);
         }
         else if(stricmp(cmd,"NEXT")==0)
         {
            command_next(&var);
         }
         else if(stricmp(cmd,"POST")==0)
         {
            command_post(&var);
         }
         else if(stricmp(cmd,"SLAVE")==0)
         {
            socksendtext(&var,"202 Slave status noted (but ignored)" CRLF);
         }
         else if(stricmp(cmd,"QUIT")==0)
         {
            socksendtext(&var,"205 Goodbye" CRLF);
            var.disconnect=1;
         }
         else if(stricmp(cmd,"XOVER")==0)
         {
            command_xover(&var);
         }
         else
         {
            socksendtext(&var,"500 Unknown command" CRLF);
         }
      }
   }

   os_logwrite("(%s) Connection closed",var.clientid);

   if(var.openmb)
   {
      JAM_CloseMB(var.openmb);
      free(var.openmb);
   }

   freegroups(&var);
   freexlat(&var);

   freesockio(var.sio);

	os_getexclusive();
	server_openconnections--;
	os_stopexclusive();

   shutdown(s,2);
   close(s);
}

/*

   om man inplar netmail:

   command_abhs och command_xover() kollar toname/fromname

   command_post anv„nder en rad f”rst i meddelanden: "To: namn,adress".
   rejectar mails som saknar den h„r raden.

   netmail inte tillg„nligt f”r icke-inloggade. g† igenom alla loopar med
   for(g=firstgroup;g;g=g->next) s† att de undviker netmail.

   config i usersfil

   wbunaarf password   A    A     "Johannes Nilsson,wbunaarf"
   billing  password   A    AX    "Johan Billing,*"

   om man inte uppdaterar jamgetminmaxnum() till att söka efter toname/fromname 
   för att sätta min och max, vad händer då om det kommer nya texter som inte är
   till en själv? toname går bra, men det är svårt att söka efer fromname på ett 
   snabbt sätt.
   
*/



