#include "nntpserv.h"

bool login(struct var *var,uchar *user,uchar *pass)
{
   FILE *fp;
   uchar s[1000],cfguser[100],cfgpass[100],cfgreadgroups[50],cfgpostgroups[50];
   ulong pos;

   if(!(fp=fopen(cfg_usersfile,"r")))
   {
      os_logwrite("(%s) Can't read users file %s",var->clientid,cfg_usersfile);
      return(FALSE);
   }

   while(fgets(s,999,fp))
   {
      strip(s);
      pos=0;

      if(s[0]!=0 && s[0]!='#')
      {
         getcfgword(s,&pos,cfguser,100);
         getcfgword(s,&pos,cfgpass,100);
         getcfgword(s,&pos,cfgreadgroups,50);
         getcfgword(s,&pos,cfgpostgroups,50);

         if(stricmp(cfguser,user) == 0)
         {
            if(strcmp(cfgpass,pass)!=0)
            {
               os_logwrite("(%s) Wrong password for %s",var->clientid,user);
               fclose(fp);
               return(FALSE);
            }

            os_logwrite("(%s) Logged in as %s",var->clientid,user);

            strcpy(var->readgroups,cfgreadgroups);
            strcpy(var->postgroups,cfgpostgroups);

            fclose(fp);

            return(TRUE);
         }
      }
   }

   os_logwrite("(%s) Unknown user %s",var->clientid,user);

   fclose(fp);

   return(FALSE);
}
