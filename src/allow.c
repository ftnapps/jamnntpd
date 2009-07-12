#include "nntpserv.h"

bool compareip(uchar *ip,uchar *pat)
{
   int c;

   for(c=0;pat[c];c++)
   {
      if(pat[c]=='*')
         return(TRUE);

      if(ip[c] != pat[c])
         return(FALSE);
   } 

   return(TRUE);
}

bool checkallow(struct var *var,uchar *str)
{
   FILE *fp;
   uchar s[1000];
   long c,d;

   if(!(fp=fopen(cfg_allowfile,"r")))
   {
      os_logwrite("(%s) Can't read allow file %s",var->clientid,cfg_allowfile);
      return(FALSE);
   }

   while(fgets(s,999,fp))
   {
      strip(s);

      if(s[0]!=0 && s[0]!='#')
      {
         for(c=0;!isspace(s[c]) && s[c]!=0;c++);
         if(isspace(s[c])) s[c++]=0;
         while(isspace(s[c])) c++;

         for(d=c;!isspace(s[d]) && s[d]!=0;d++);
         if(isspace(s[d])) s[d++]=0;
         while(isspace(s[d]))  d++;

         if(compareip(str,s))
         {
            mystrncpy(var->readgroups,&s[c],50);
            mystrncpy(var->postgroups,&s[d],50);

            fclose(fp);
            return(TRUE);
         }
      }
   }

   fclose(fp);
   return(FALSE);
}
