#include "nntpserv.h"

bool readgroups(struct var *var)
{
   FILE *fp;
   uchar s[1000],tagname[100],group[2],aka[40],jampath[100];
   bool res1,res2,res3,res4;
   ulong pos,line;
   struct group *newgroup,*lastgroup;

   if(!(fp=fopen(cfg_groupsfile,"r")))
   {
      os_logwrite("(%s) Failed to read group configuration file %s",var->clientid,cfg_groupsfile);
      return(FALSE);
   }

   lastgroup=NULL;
   var->firstgroup=NULL;

   line=0;

   while(fgets(s,999,fp))
   {
      line++;
      strip(s);
      pos=0;
      
      if(s[0]!=0 && s[0]!='#')
      {
         res1=getcfgword(s,&pos,tagname,100);
         res2=getcfgword(s,&pos,group,2);
         res3=getcfgword(s,&pos,aka,40);
         res4=getcfgword(s,&pos,jampath,100);

         if(res1 && res2 && res3 && res4)
         {
            if(!(newgroup=(struct group *)malloc(sizeof(struct group))))
            {
               fclose(fp);
               return(FALSE);
            }

            newgroup->next=NULL;
            if(!var->firstgroup) var->firstgroup=newgroup;
            if(lastgroup) lastgroup->next=newgroup;
            lastgroup=newgroup;

            strcpy(newgroup->tagname,tagname);
            newgroup->group=group[0];
            strcpy(newgroup->aka,aka);
            strcpy(newgroup->jampath,jampath);
         }
         else
         {
            os_logwrite("(%s) Syntax error on line %lu in %s, skipping line",var->clientid,line,cfg_groupsfile);
         }
      }
   }

   fclose(fp);

   if(!var->firstgroup)
   {
      os_logwrite("(%s) No groups configured",var->clientid);
      return(FALSE);
   }

   return(TRUE);
}

void freegroups(struct var *var)
{
   struct group *gr,*gr2;

   gr=var->firstgroup;

   while(gr)
   {
      gr2=gr->next;
      free(gr);
      gr=gr2;
   }
}

