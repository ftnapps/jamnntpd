#include "nntpserv.h"

bool checkallow(struct var *var,uchar *ip)
{
   FILE *fp;
   uchar s[1000],cfgip[100],cfgreadgroups[50],cfgpostgroups[50];
   int res1,res2,res3;
   ulong pos,line;
   
   if(!(fp=fopen(cfg_allowfile,"r")))
   {
      os_logwrite("(%s) Can't read allow file %s",var->clientid,cfg_allowfile);
      return(FALSE);
   }

   line=0;
   
   while(fgets(s,999,fp))
   {
      line++;
      strip(s);
      pos=0;

      if(s[0]!=0 && s[0]!='#')
      {
         res1=getcfgword(s,&pos,cfgip,100);
         res2=getcfgword(s,&pos,cfgreadgroups,50);
         res3=getcfgword(s,&pos,cfgpostgroups,50);

         if(res1)
         {
            if(matchpattern(cfgip,ip))
            {
               if(res2) strcpy(var->readgroups,cfgreadgroups);
               if(res3) strcpy(var->postgroups,cfgpostgroups);

               fclose(fp);
               return(TRUE);
            }
         }
         else
         {
            os_logwrite("(%s) Syntax error on line %lu in %s, skipping line",var->clientid,line,cfg_allowfile);
         }
      }
   }

   fclose(fp);
   return(FALSE);
}
