#include "nntpserv.h"

void mystrncpy(uchar *dest,uchar *src,long len)
{
   if(len == 0)
      return;
      
   strncpy(dest,src,(size_t)len-1);
   dest[len-1]=0;
}

void strip(uchar *str)
{
   int c;

   if(str[0] == 0)
      return;

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) 
		str[c]=0;
}

bool setboolonoff(uchar *opt,bool *var)
{
   if(stricmp(opt,"on")==0)
   {
      *var=TRUE;
      return(TRUE);
   }

   if(stricmp(opt,"off")==0)
   {
      *var=FALSE;
      return(TRUE);
   }

   return(FALSE);
}

bool getcfgword(uchar *line, ulong *pos, uchar *dest, ulong destlen)
{
   bool quote;
   ulong begin;

   quote=FALSE;

   while(isspace(line[*pos]) && line[*pos]!=0)
      (*pos)++;

   if(line[*pos] == 0)
      return(FALSE);

   if(line[*pos] == '"')
   {
      quote=TRUE;
      (*pos)++;
   }

   begin=*pos;

   while(line[*pos]!=0 && !(isspace(line[*pos]) && !quote) && !(line[*pos] == '"' && quote))
      (*pos)++;

   if(line[*pos] != 0)
   {
      line[*pos]=0;
      (*pos)++;
   }

   mystrncpy(dest,&line[begin],destlen);

   return(TRUE);
}

bool getcomma(uchar *line, ulong *pos, uchar *dest, ulong destlen)
{
   ulong c,d;

   c=*pos;
   d=0;

   for(;;)
   {
      if(line[c] == 0 || line[c] == ',')
      {
         if(line[c] != 0) c++;
         *pos=c;
         dest[d]=0;

         if(dest[0]) return(TRUE);
         else        return(FALSE);
      }
      else
      {
         if(d < destlen-1)
            dest[d++]=line[c];
      }

      c++;
   }
}

bool matchname(uchar *namelist,uchar *name)
{
   uchar namepat[100];
   ulong count;
   
   count=0;
      
   while(getcomma(namelist,&count,namepat,100))
      if(matchpattern(namepat,name)) return(TRUE);
   
   return(FALSE);
}   

bool matchgroup(uchar *groups,uchar group)
{
   int c;

   if(strcmp(groups,"*") == 0)
      return(TRUE);

   if(strcmp(groups,"-") == 0)
      return(FALSE);

   for(c=0;groups[c];c++)
      if(tolower(groups[c]) == tolower(group)) return(TRUE);

   return(FALSE);
}

bool matchpattern(uchar *pat,uchar *str)
{
   int c;

   for(c=0;pat[c];c++)
   {
      if(pat[c]=='*')
         return(TRUE);

      if(pat[c]!='?')
      {
         if(tolower(str[c]) != tolower(pat[c]))
            return(FALSE);
      }
   }

   if(str[c])
      return(FALSE);

   return(TRUE);
}

bool ispattern(uchar *pat)
{
   if(strchr(pat,'*'))
      return(TRUE);
   
   else
      return(FALSE);
}

void stripctrl(uchar *str)
{
   int c,d;

   c=0;
   d=0;

   while(str[c])
   {
      if(str[c] >= 32)
         str[d++]=str[c];

      c++;
   }

   str[d]=0;
}

ulong count8bit(uchar *text)
{
   ulong c,res;

   res=0;

   for(c=0;text[c];c++)
      if(text[c] & 0x80) res++;

   return(res);
}

void freelist(void *first)
{
   void *ptr,*next;
   
   ptr=first;
   
   while(ptr)
   {
      next=*(void **)ptr;
      free(ptr);
      ptr=next;
   }
}

uchar *getkludgedata(uchar *line)
{
   int c;
   
   for(c=0;line[c];c++)
      if(line[c] == ':' || line[c] == ' ') break;
      
   if(line[c])
      c++;
      
   while(line[c] == ' ')
      c++;
      
   return(&line[c]);
}      

      
void makedate(struct _stamp *stamp,uchar *dest,uchar *tz)
{
   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
   uchar *daynames[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
   uchar rfctz[6];
   struct tm tm,*tp;
   time_t t;
   
   DosDate_to_TmDate((union stamp_combo *)stamp,&tm);

   if(tz[0])
   {
      if(tz[0] == '-') mystrncpy(rfctz,tz,6);
      else             sprintf(rfctz,"+%.4s",tz);

      t=mktime(&tm);
      tp=gmtime(&t);
      memcpy(&tm,tp,sizeof(struct tm));
   }
   else
   {
      strcpy(rfctz, "GMT");

      /* Rebuild tm - DosDate_to_TmDate does not calculate wday */
      t=mktime(&tm);
      tp=localtime(&t);
      memcpy(&tm,tp,sizeof(struct tm));
   }

   sprintf(dest,"%s, %d %s %d %02d:%02d:%02d %s",
      daynames[tm.tm_wday],
      tm.tm_mday,
      monthnames[tm.tm_mon],
      1900+tm.tm_year,
      tm.tm_hour,
      tm.tm_min,
      tm.tm_sec,
      rfctz);
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

void stripchrs(uchar *str)
{
   /* remove charset level */
   
   if(strchr(str,' '))
      *strchr(str,' ')=0;
      
   strip(str);
}

void extractorigin(uchar *text,uchar *addr)
{
   ulong textpos,d;
   uchar originbuf[100];
     
   textpos=0;
             
   while(text[textpos])
   {
      d=textpos;

      while(text[d] != 13 && text[d] != 0)
         d++;

      if(text[d] == 13)
         d++;

      if(d-textpos > 11 && strncmp(&text[textpos]," * Origin: ",11)==0)
         mystrncpy(originbuf,&text[textpos],min(d-textpos,100));

      textpos=d;
   }

   if(originbuf[0])
   {
      /* Find address part */
   
      d=strlen(originbuf);

      while(d>0 && originbuf[d]!='(') 
         d--;

      if(originbuf[d] == '(')
      {
         strcpy(addr,&originbuf[d+1]);
   
         if(strchr(addr,')'))
            *strchr(addr,')')=0;
      }
   }
}
