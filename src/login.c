#include "nntpserv.h"

bool login(struct var *var,uchar *user,uchar *pass)
{
   FILE *fp;
   uchar s[1000],cfguser[100],cfgpass[100],cfgreadgroups[50],cfgpostgroups[50],dispname[36];
   int res1,res2,res3,res4,res5;
   ulong pos,line;

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
         res1=getcfgword(s,&pos,cfguser,100);
         res2=getcfgword(s,&pos,cfgpass,100);
         res3=getcfgword(s,&pos,cfgreadgroups,50);
         res4=getcfgword(s,&pos,cfgpostgroups,50);
         res5=getcfgword(s,&pos,dispname,36);

         if(res1 && res2 && res3 && res4)
         {
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
               if(res5) strcpy(var->dispname,dispname);
               var->login=TRUE;
            
               fclose(fp);

               return(TRUE);
            }
         }
         else
         {
            os_logwrite("(%s) Syntax error on line %lu in %s, skipping line",var->clientid,line,cfg_usersfile);
         }
      }
   }

   os_logwrite("(%s) Unknown user %s",var->clientid,user);

   fclose(fp);

   return(FALSE);
}
