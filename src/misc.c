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

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) 
		str[c]=0;
}

void makedate(time_t t,uchar *dest,uchar *tz)
{
   time_t t1,t2;
   struct tm *tp;
   ulong jam_utcoffset;
   uchar rfctz[6];

   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
   uchar *daynames[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

   /* Some timezone tricks */

   t1=time(NULL);
   tp=gmtime(&t1);
   tp->tm_isdst=-1;
   t2=mktime(tp);
   jam_utcoffset=t2-t1;
   t1=t+jam_utcoffset;

   if(tz[0])
   {
      if(tz[0] == '-')
         mystrncpy(rfctz,tz,6);
      else
         sprintf(rfctz,"+%.4s",tz);
   }
   else
   {
      strcpy(rfctz, "GMT");
      t1=t1+jam_utcoffset;
   }

   tp=localtime(&t1);

   sprintf(dest,"%s, %d %s %d %02d:%02d:%02d %s",
      daynames[tp->tm_wday],
      tp->tm_mday,
      monthnames[tp->tm_mon],
      1900+tp->tm_year,
      tp->tm_hour,
      tp->tm_min,
      tp->tm_sec,
      rfctz);
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

      if(tolower(str[c]) != tolower(pat[c]))
         return(FALSE);
   }

   if(str[c])
      return(FALSE);

   return(TRUE);
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
