#include "nntpserv.h"

void mystrncpy(uchar *dest,uchar *src,long len)
{
   strncpy(dest,src,(size_t)len-1);
   dest[len-1]=0;
}

void strip(uchar *str)
{
   int c;

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) 
		str[c]=0;
}

void makedate(time_t t,uchar *dest)
{
   time_t t1,t2;
   struct tm *tp;
   ulong jam_utcoffset;

   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
   uchar *daynames[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

   /* Some timezone tricks */

   t1=time(NULL);
   tp=gmtime(&t1);
   tp->tm_isdst=-1;
   t2=mktime(tp);
   jam_utcoffset=t2-t1;

   t1=t+2*jam_utcoffset; 
   tp=localtime(&t1);

   sprintf(dest,"%s, %d %s %d %02d:%02d:%02d GMT",
      daynames[tp->tm_wday],
      tp->tm_mday,
      monthnames[tp->tm_mon],
      1900+tp->tm_year,
      tp->tm_hour,
      tp->tm_min,
      tp->tm_sec);
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
