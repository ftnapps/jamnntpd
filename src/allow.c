#include "nntpserv.h"

bool checkallow(struct var *var,uchar *ip)
{
   FILE *fp;
   uchar s[1000],cfgip[100],cfgreadgroups[50],cfgpostgroups[50];
   ulong pos;

   if(!(fp=fopen(cfg_allowfile,"r")))
   {
      os_logwrite("(%s) Can't read allow file %s",var->clientid,cfg_allowfile);
      return(FALSE);
   }

   while(fgets(s,999,fp))
   {
      strip(s);
      pos=0;

      if(s[0]!=0 && s[0]!='#')
      {
         getcfgword(s,&pos,cfgip,100);
         getcfgword(s,&pos,cfgreadgroups,50);
         getcfgword(s,&pos,cfgpostgroups,50);

         if(matchpattern(cfgip,ip))
         {
            strcpy(var->readgroups,cfgreadgroups);
            strcpy(var->postgroups,cfgpostgroups);

            fclose(fp);
            return(TRUE);
         }
      }
   }

   fclose(fp);
   return(FALSE);
}
