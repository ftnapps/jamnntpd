#include "nntpserv.h"

ulong cfg_port        = CFG_PORT;
ulong cfg_maxconn     = CFG_MAXCONN;

uchar *cfg_allowfile  = CFG_ALLOWFILE;
uchar *cfg_groupsfile = CFG_GROUPSFILE;
uchar *cfg_logfile    = CFG_LOGFILE;
uchar *cfg_usersfile  = CFG_USERSFILE;

bool cfg_def_flowed = CFG_DEF_FLOWED;
bool cfg_def_showto = CFG_DEF_SHOWTO;

bool cfg_debug;
bool cfg_noxlat;
bool cfg_noecholog;
bool cfg_nostripre;
bool cfg_noreplyaddr;
bool cfg_notearline;

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
   }

   if(JAM_OpenMB(group->jampath,&var->openmb))
   {
      if(var->openmb) free(var->openmb);
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

   socksendtext(var,"215 List of newsgroups follows" CRLF);

   for(g=var->firstgroup;g && !var->disconnect && !get_server_quit();g=g->next)
   {
      if(strchr(var->readgroups,g->group))
      {
         if(!jamgetminmaxnum(var,g,&min,&max,&num))
         {
            min=0;
            max=0;
            num=0;
         }

         if(strchr(var->postgroups,g->group))
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
      if(strchr(var->readgroups,g->group) && stricmp(g->tagname,groupname)==0) break;

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

   sockprintf(var,"223 %lu <%lu%%%s@JamNNTP> Article retrieved" CRLF,
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

   sockprintf(var,"223 %lu <%lu%%%s@JamNNTP> Article retrieved" CRLF,
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
#define LINE_WIDTH 78 /* leaves room for trailing space */
#define MAX_WIDTH 997

void copyline(uchar *dest,uchar *src,long len)
{
   int d,c;

   d=0;

   for(c=0;c<len;c++)
      if(src[c] != 0x8d && src[c] != 10) dest[d++]=src[c];

   dest[d]=0;
}

void sendtextblock(struct var *var,uchar *text)
{
   long c,d,textpos,lastspace;
   uchar buf[1000],buf2[1000];
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
         if(text[textpos+c] == 32 || text[textpos+c] == 13) c++;
         textpos+=c;
      }

      /* Code for format=flowed */

      if(var->opt_flowed && strcmp(buf,"-- ")!=0)
      {
         strip(buf);
         if(wrapped) strcat(buf," "); /* For format=flowed */

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
      socksendtext(var,buf);
   }
}

void command_abhs(struct var *var,uchar *cmd)
{
   uchar *article;
   ulong articlenum;
   struct group *group;
   ulong min,max,num;
   uchar datebuf[50],fromname[100],fromaddr[100],toname[100],toaddr[100];
   uchar replyaddr[100],subject[100],buf[500],fromfromaddr[100];
   uchar *at,*pc;
   bool xlat;

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
      pc=strchr(article,'%');

      if(!at || !pc)
      {
         socksendtext(var,"430 No such article found" CRLF);
         return;
      }

      *at=0;
      *pc=0;

      at++;
      pc++;

      if(strcmp(at,"JamNNTP") != 0)
      {
         socksendtext(var,"430 No such article found" CRLF);
         return;
      }

      for(group=var->firstgroup;group;group=group->next)
         if(strchr(var->readgroups,group->group) && stricmp(pc,group->tagname) == 0) break;

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
      sockprintf(var,"223 %lu <%lu%%%s@JamNNTP> Article retrieved" CRLF,
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

   if(!(subpack=JAM_NewSubPacket()))
   {
      socksendtext(var,"503 Local error: JAM_NewSubPacket() failed" CRLF);
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

   if(stricmp(cmd,"ARTICLE")==0)
      sockprintf(var,"220 %ld <%ld%%%s@JamNNTP> Article retrieved - Head and body follow" CRLF,articlenum,articlenum,group->tagname);
            
   if(stricmp(cmd,"HEAD")==0)
      sockprintf(var,"221 %ld <%ld%%%s@JamNNTP> Article retrieved - Head follows" CRLF,articlenum,articlenum,group->tagname);

   if(stricmp(cmd,"BODY")==0)
      sockprintf(var,"222 %ld <%ld%%%s@JamNNTP> Article retrieved - Body follows" CRLF,articlenum,articlenum,group->tagname);

   xlat=TRUE;

   count=0;

   while((field=JAM_GetSubfield_R(subpack,&count)))
   {
      if(field->LoID == JAMSFLD_FTSKLUDGE)
      {
			mystrncpy(buf,field->Buffer,min(field->DatLen+1,200));

         if(strnicmp(buf,"CHRS",4)==0)
            if(strstr(buf,"LATIN")) xlat=FALSE;
      }
   }

   if(stricmp(cmd,"ARTICLE") == 0 || stricmp(cmd,"HEAD") == 0)
   {
      count=0;

      fromname[0]=0;
      fromaddr[0]=0;
      toname[0]=0;
      toaddr[0]=0;
      subject[0]=0;
      replyaddr[0]=0;

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

            case JAMSFLD_FTSKLUDGE:
               if(field->DatLen > 10)
               {
                  if(strnicmp(field->Buffer,"REPLYADDR ",10)==0)
                     mystrncpy(replyaddr,&field->Buffer[10],min(field->DatLen+1-10,100));

                  if(strnicmp(field->Buffer,"REPLYADDR: ",11)==0)
                     mystrncpy(replyaddr,&field->Buffer[11],min(field->DatLen+1-11,100));
               }

               break;
         }
      }

      if(fromaddr[0] == 0) strcpy(fromaddr,"unknown");
      if(fromname[0] == 0) strcpy(fromname,"unknown");
      if(toname[0] == 0)   strcpy(toname,"(none)");

      if(xlat && !cfg_noxlat)
      {
         ibmtolatin(fromname);
         ibmtolatin(toname);
         ibmtolatin(subject);
      }

      makedate(header.DateWritten,datebuf);

      sockprintf(var,"Path: JamNNTP!not-for-mail" CRLF);

      if(replyaddr[0]) strcpy(fromfromaddr,replyaddr);
      else             strcpy(fromfromaddr,fromaddr);

      if(var->opt_showto)
         sockprintf(var,"From: \"%s -> %s\" <%s>" CRLF,fromname,toname,fromfromaddr);

      else
         sockprintf(var,"From: \"%s\" <%s>" CRLF,fromname,fromfromaddr);

      sockprintf(var,"Newsgroups: %s" CRLF,group->tagname);
      sockprintf(var,"Subject: %s" CRLF,subject);
      sockprintf(var,"Date: %s" CRLF,datebuf);
      sockprintf(var,"Message-ID: <%ld%%%s@JamNNTP>" CRLF,articlenum,group->tagname);

      if(header.ReplyTo)
         sockprintf(var,"References: <%ld%%%s@JamNNTP>" CRLF,header.ReplyTo,group->tagname);

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

      if(var->opt_flowed)
         socksendtext(var,"Content-Type: text/plain; charset=iso-8859-1; format=flowed" CRLF);

      else
         socksendtext(var,"Content-Type: text/plain; charset=iso-8859-1; format=fixed" CRLF);

      socksendtext(var,"Content-Transfer-Encoding: 8bit" CRLF);
   }

   if(stricmp(cmd,"ARTICLE") == 0)
      socksendtext(var,CRLF);

   if(stricmp(cmd,"ARTICLE") == 0 || stricmp(cmd,"BODY") == 0)
   {
      uchar *text;

      if((text=malloc(header.TxtLen+1)))
      {
         if(header.TxtLen)
         {
            res=JAM_ReadMsgText(var->openmb,header.TxtOffset,header.TxtLen,text);
            text[header.TxtLen]=0;

            if(!res)
            {
               if(xlat && !cfg_noxlat)
                  ibmtolatin(text);

               sendtextblock(var,text); 
            }
         }

         free(text);
      }
   }

   socksendtext(var,"." CRLF);

   JAM_DelSubPacket(subpack);
}

void escapetab(uchar *str)
{
   int c;

   for(c=0;str[c];c++)
      if(str[c] == '\t') str[c]=' ';
}

void command_xover(struct var *var)
{
   uchar *article,*dash;
   ulong min,max,num;
   ulong first,last,c;
   uchar msgid[100],reply[100],buf[100];
   uchar datebuf[50],fromname[100],toname[100],fromaddr[100],subject[100];
   bool xlat;

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

   if(first > last || last-min < 1)
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
      if((subpack=JAM_NewSubPacket()))
      {
         res=JAM_ReadMsgHeader(var->openmb,c-baseheader.BaseMsgNum,&header,&subpack);

         if(res == 0)
         {
            if(!(header.Attribute & MSG_DELETED))
            {
               count=0;

               xlat=TRUE;

               fromname[0]=0;
               fromaddr[0]=0;
               subject[0]=0;
               toname[0]=0;

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

                     case JAMSFLD_FTSKLUDGE:
                        mystrncpy(buf,field->Buffer,min(field->DatLen+1,100));

                        if(strnicmp(buf,"CHRS",4)==0)
                           if(strstr(buf,"LATIN")) xlat=FALSE;

                        break;
                  }
               }

               if(toname[0] == 0)
                  strcpy(toname,"(none)");

               if(xlat && !cfg_noxlat)
               {
                  ibmtolatin(fromname);
                  ibmtolatin(toname);
                  ibmtolatin(subject);
               }

               makedate(header.DateWritten,datebuf);

               sprintf(msgid,"<%ld%%%s@JamNNTP>",c,var->currentgroup->tagname);

               reply[0]=0;

               if(header.ReplyTo)
                  sprintf(reply,"<%ld%%%s@JamNNTP>",header.ReplyTo,var->currentgroup->tagname);

               escapetab(fromname);
               escapetab(toname);
               escapetab(fromaddr);
               escapetab(subject);

               if(var->opt_showto)
               {
                  sockprintf(var,"%ld\t%s\t%s -> %s <%s>\t%s\t%s\t%s\t\t" CRLF,
                     c,subject,fromname,toname,fromaddr,datebuf,msgid,reply);
               }
               else
               {
                  sockprintf(var,"%ld\t%s\t%s <%s>\t%s\t%s\t%s\t\t" CRLF,
                     c,subject,fromname,fromaddr,datebuf,msgid,reply);
               }
            }
         }

         JAM_DelSubPacket(subpack);
      }
   }

   socksendtext(var,"." CRLF);
}

#define POST_MAXSIZE 10000

void addjamfield(s_JamSubPacket *SubPacket_PS,ulong fieldnum,uchar *fielddata)
{
   s_JamSubfield	Subfield_S;

   Subfield_S.LoID   = fieldnum;
   Subfield_S.HiID   = 0;
   Subfield_S.DatLen = strlen(fielddata);
   Subfield_S.Buffer = fielddata;

   JAM_PutSubfield( SubPacket_PS, &Subfield_S);
}

uchar hextodec(uchar c1,uchar c2)
{
   uchar *f1,*f2;

   uchar *hex="0123456789ABCDEF";

   f1=strchr(hex,c1);
   f2=strchr(hex,c2);

   if(!f1 || !f2)
      return('?');

   return((f1-hex)*16+(f2-hex));
}

/* Only Quoted-Printable, base64 is not supported */
void unmime(uchar *txt)
{
   int c,d;
   bool mime,qp;

   d=0;
   c=0;
   mime=FALSE;
   qp=FALSE;

   while(txt[c])
   {
      if(txt[c]=='=' && txt[c+1]=='?')
      {
         mime=TRUE;
         qp=FALSE;
         c+=2;

         while(txt[c]!='?' && txt[c]!=0) c++;
         if(txt[c]) c++;

         if(txt[c] == 'Q')
            qp=TRUE;

         while(txt[c]!='?' && txt[c]!=0) c++;
         if(txt[c]) c++;
      }
      else if(mime && txt[c]=='?' && txt[c+1]=='=')
      {
         mime=FALSE;
         c+=2;
      }
      else if(mime && qp && txt[c]=='_')
      {
         txt[d++]=' ';
         c++;
      }
      else if(mime && qp && txt[c]=='=' && txt[c+1] && txt[c+2])
      {
         txt[d++]=hextodec(txt[c+1],txt[c+2]);
         c+=3;
      }
      else
      {
         txt[d++]=txt[c++];
      }
   }

   txt[d]=0;
}

void getparentmsgidfromnum(struct var *var,uchar *article,uchar *groupname,uchar *msgid,uchar *from,ulong *oldnum)
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

   msgid[0]=0;
   from[0]=0;
   *oldnum=0;

   if(article[0] != '<' || article[strlen(article)-1] != '>')
      return;

   strcpy(article,&article[1]);
   article[strlen(article)-1]=0;

   at=strchr(article,'@');
   pc=strchr(article,'%');

   if(!at || !pc)
      return;

   *at=0;
   *pc=0;

   at++;
   pc++;

   if(strcmp(at,"JamNNTP") != 0)
      return;

   if(stricmp(pc,groupname) == 0)
      *oldnum=atol(article);

   for(group=var->firstgroup;group;group=group->next)
      if(strchr(var->readgroups,group->group) && stricmp(pc,group->tagname) == 0) break;

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

   if(!(subpack=JAM_NewSubPacket()))
   {
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
   }

   JAM_DelSubPacket(subpack);
   return;
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
   uchar *text,line[1000];
   ulong textlen,textpos,c,d,parentmsg;
   bool finished,toobig;
   uchar from[100],fromaddr[100],toname[100],subject[100],organization[100],newsgroup[100];
   uchar contenttype[100],contenttransferencoding[100],reference[100],newsreader[100];
   uchar msgid[100],replyid[100],replyto[100];
   struct group *g;
   s_JamSubPacket*	SubPacket_PS;
   s_JamMsgHeader	Header_S;
   time_t t1,t2;
   struct tm *tp;
   int res;
   bool flowed;

   if(!(text=malloc(POST_MAXSIZE+200)))
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

   textpos=0;
   textlen=strlen(text);

   finished=FALSE;

   from[0]=0;
   fromaddr[0]=0;
   subject[0]=0;
   contenttype[0]=0;
   contenttransferencoding[0]=0;
   newsgroup[0]=0;
   reference[0]=0;
   organization[0]=0;
   replyto[0]=0;
   newsreader[0]=0;

   flowed=FALSE;

   while(textpos < textlen && !finished)
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
            if(text[textpos+1] == ' ' || text[textpos+1] == '\t')
            {
               /* Multi-line header */

               while(text[textpos+1] == ' ' || text[textpos+1] == '\t')
                  textpos++;

               c--; /* Remove LF again */
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
         if(line[strlen(line)-1] == ')' && strchr(line,'('))
         {
            /* From: mark@cbosgd.ATT.COM (Mark Horton) */

            line[strlen(line)-1]=0;
            mystrncpy(from,strrchr(line,'(')+1,100);
            strip(from);

            *strrchr(line,'(')=0;
            mystrncpy(fromaddr,&line[6],100);
            strip(fromaddr);
         }
         else if(line[strlen(line)-1] == '>' && strchr(line,'<'))
         {
            /* From: Mark Horton <mark@cbosgd.ATT.COM> */

            line[strlen(line)-1]=0;
            mystrncpy(fromaddr,strrchr(line,'<')+1,100);
            strip(fromaddr);
            
            *strchr(line,'<')=0;
            mystrncpy(from,&line[6],100);
            strip(from);
         }
         else
         {
            mystrncpy(from,&line[6],100);
            mystrncpy(fromaddr,&line[6],100);
         }

         if(strlen(from) > 0)
            if(from[0] == '\"' && from[strlen(from)-1]=='\"')
            {
               from[strlen(from)-1]=0;
               strcpy(from,&from[1]);
            }
      }
      else if(strnicmp(line,"Newsgroups: ",12)==0)
      { 
         strip(line);
         mystrncpy(newsgroup,&line[12],100);
      }
      else if(strnicmp(line,"Subject: ",9)==0)
      {
         mystrncpy(subject,&line[9],100);
      }
      else if(strnicmp(line,"Reply-To: ",10)==0)
      {
         mystrncpy(replyto,&line[10],100);
      }
      else if(strnicmp(line,"Content-Type: ",14)==0)
      {
         flowed=FALSE;

         if(strstr(line,"format=flowed"))
            flowed=TRUE;

         if(strchr(line,';'))
            *strchr(line,';')=0;

         strip(line);
         mystrncpy(contenttype,&line[14],100);
      }
      else if(strnicmp(line,"Content-Transfer-Encoding: ",27)==0)
      {
         if(strchr(line,';'))
            *strchr(line,';')=0;

         strip(line);
         mystrncpy(contenttransferencoding,&line[27],100);
      }
      else if(strnicmp(line,"References: ",12)==0)
      {
         if(strrchr(line,'<'))
            mystrncpy(reference,strrchr(line,'<'),100);
      }
      else if(strnicmp(line,"Organization: ",14)==0)
      {
         mystrncpy(organization,&line[14],100);
      }
      else if(strnicmp(line,"X-Newsreader: ",14)==0)
      {
         mystrncpy(newsreader,&line[14],100);
      }
      else if(strnicmp(line,"User-Agent: ",12)==0)
      {
         mystrncpy(newsreader,&line[12],100);
      }
      else if(line[0] == 0)
      {
         finished=TRUE; /* End of headers */
      }
   }

   unmime(from);
   unmime(subject);
   unmime(organization);

   if(!cfg_nostripre && (strncmp(subject,"Re: ",4)==0 || strcmp(subject,"Re:")==0))
      strcpy(subject,&subject[4]);

   from[36]=0;
   subject[72]=0;
   organization[70]=0;
   newsreader[75]=0;

   d=0;

   while(textpos < textlen)
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

      if(flowed && line[0]!=0 && line[0]!='>')
      {
         if(line[0] == ' ')
            strcpy(line,&line[1]);

         if(line[strlen(line)-1] == ' ')
         {
            strcpy(&text[d],line);
            d+=strlen(line);
         }
         else
         {
            strcpy(&text[d],line);
            d+=strlen(line);
            text[d++]=13;
         }
      }
      else
      {
         strcpy(&text[d],line);
         d+=strlen(line);
         text[d++]=13;
      }
   }

   while(d > 0 && text[d-1] == 13) d--;

   if(d > 0)
   {
      text[d++]=13;
      text[d++]=13;
   }

   text[d]=0;

   if(contenttype[0] && stricmp(contenttype,"text/plain")!=0)
   {
      sockprintf(var,"441 Posting failed (Content-Type %s not allowed, please use text/plain)" CRLF,contenttype);
      os_logwrite("(%s) POST failed (Content-Type %s not allowed)",var->clientid,contenttype);
      free(text);
      return;
   }

   if(contenttransferencoding[0] && stricmp(contenttransferencoding,"8bit")!=0 && stricmp(contenttransferencoding,"7bit")!=0)
   {
      sockprintf(var,"441 Posting failed (Content-Transfer-Encoding %s not allowed, please use 7bit or 8bit)" CRLF,contenttransferencoding);
      os_logwrite("(%s) POST failed (Content-Transfer-Encoding %s not allowed)",var->clientid,contenttransferencoding);
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

   for(g=var->firstgroup;g;g=g->next)
      if(stricmp(newsgroup,g->tagname)==0) break;

   if(!g)
   {
      sockprintf(var,"441 Posting failed (Unknown newsgroup %s)" CRLF,newsgroup);
      os_logwrite("(%s) POST failed (Unknown newsgroup %s)",var->clientid,newsgroup);
      free(text);
      return;
   }

   if(!(strchr(var->postgroups,g->group)))
   {
      sockprintf(var,"441 Posting failed (Posting access denied to %s)" CRLF,newsgroup);
      os_logwrite("(%s) POST failed (Posting access denied to %s)",var->clientid,newsgroup);
      free(text);
      return;
   }

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

   Header_S.DateWritten   = time(NULL)-t2+t1;
   Header_S.DateReceived  = time(NULL)-t2+t1;
   Header_S.DateProcessed = time(NULL)-t2+t1;

   replyid[0]=0;
   msgid[0]=0;
   parentmsg=0;
   toname[0]=0;

   sprintf(msgid,"%s %08lx",g->aka,get_msgid_num());
   addjamfield(SubPacket_PS,JAMSFLD_MSGID,msgid);

   if(reference[0])
   {
      getparentmsgidfromnum(var,reference,g->tagname,replyid,toname,&parentmsg);

      addjamfield(SubPacket_PS,JAMSFLD_REPLYID,replyid);
      Header_S.ReplyTo  = parentmsg;

      toname[36]=0;
   }
   else
   {
      strcpy(toname,"All");
   }

   Header_S.MsgIdCRC = JAM_Crc32(msgid,strlen(msgid));
   Header_S.ReplyCRC = JAM_Crc32(replyid,strlen(replyid));

   if(newsreader[0]==0 || cfg_notearline) strcpy(line,"---" CR);
   else sprintf(line,"--- %s" CR,newsreader);
   strcat(text,line);

   sprintf(line," * Origin: %s (%s)" CR,organization,g->aka);
   strcat(text,line);

   if(!cfg_noxlat)
   {
      latintoibm(from);
      latintoibm(subject);
      latintoibm(text);
   }

   addjamfield(SubPacket_PS,JAMSFLD_SENDERNAME,from);
   addjamfield(SubPacket_PS,JAMSFLD_RECVRNAME,toname);
   addjamfield(SubPacket_PS,JAMSFLD_SUBJECT,subject);
   addjamfield(SubPacket_PS,JAMSFLD_OADDRESS,g->aka);

   if(replyto[0]) sprintf(line,"REPLYADDR %s",replyto);
   else           sprintf(line,"REPLYADDR %s",fromaddr);

   if(!cfg_noreplyaddr)
      addjamfield(SubPacket_PS,JAMSFLD_FTSKLUDGE,line);

   strcpy(line,SERVER_NAME " " SERVER_PIDVERSION);
   addjamfield(SubPacket_PS,JAMSFLD_PID,line);

   Header_S.Attribute = MSG_LOCAL | MSG_TYPEECHO;

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
      socksendtext(&var,"502 Maximum number of connections reached, please try again later" CRLF);
      os_logwrite("(%s) Access denied (server full)",var.clientid);

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
            socksendtext(&var,"JamNNTP supports most of RFC-977 and also has support for AUTHINFO and" CRLF);
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
            socksendtext(&var,"235 Warning: NEWGROUPS not implemented, returning empty list" CRLF);
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
   freesockio(var.sio);

	os_getexclusive();
	server_openconnections--;
	os_stopexclusive();

   shutdown(s,2);
   close(s);
}

/*

   om man inplar netmail:

   jamgetminmaxnum() s”ker efter toname f”r att s„tta min och max

   command_abhs och command_xover() kollar toname

   command_post anv„nder en rad f”rst i meddelanden: "To: namn,adress".
   rejectar mails som saknar den h„r raden.

   netmail inte tillg„nligt f”r icke-inloggade. g† igenom alla loopar med
   for(g=firstgroup;g;g=g->next) s† att de undviker netmail.

   config i usersfil

   wbunaarf password   A    A     Johannes Nilsson,wbunaarf
   billing  password   A    AX    *

*/

/*

   Nytt i 0.3:
   
   -noreplyaddr

   -s„tter x-newsreader/user-agent som tearline och jamnntpd som PID,
    tearline kan disablas med -notearline

   -g”r lookup p† adresser som connectar

   -„ndrade msgid-skapningalgoritmen en aning

   -bytte ut -noflow och -notinfrom mot -def_flowed och -def_showto

   -anv„ndare kan s„tta flowed och showto genom att logga in med options

   -f”rb„ttrade patternmatching i allow n†got. numera k”per den †tminstone
    saker som 127.0.0.* (allt efter * ignoreras dock)

   Nytt i 0.4:

   -skriver ut plattformen i PID
   -lyssnar p† ^C „ven i XOVER

*/



