#include "nntpserv.h"

bool login(struct var *var,uchar *user,uchar *pass)
{
   FILE *fp;
   uchar s[1000];
   long c,d,e;

   if(!(fp=fopen(cfg_usersfile,"r")))
   {
      os_logwrite("(%s) Can't read users file %s",var->clientid,cfg_usersfile);
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

         for(e=d;!isspace(s[e]) && s[e]!=0;e++);
         if(isspace(s[e])) s[e++]=0;
         while(isspace(s[e]))  e++;

         if(stricmp(s,user) == 0)
         {
            if(strcmp(&s[c],pass)!=0)
            {
               os_logwrite("(%s) Wrong password for %s",var->clientid,user);
               fclose(fp);
               return(FALSE);
            }

            os_logwrite("(%s) Logged in as %s",var->clientid,user);
   
            mystrncpy(var->readgroups,&s[d],50);
            mystrncpy(var->postgroups,&s[e],50);

            fclose(fp);

            return(TRUE);
         }
      }
   }

   os_logwrite("(%s) Uknown user %s",var->clientid,user);

   fclose(fp);

   return(FALSE);
}
