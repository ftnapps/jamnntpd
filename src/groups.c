#include "nntpserv.h"

bool readgroups(struct var *var)
{
   FILE *fp;
   uchar s[1000];
   long c,d,e;
   struct group *newgroup,*lastgroup;

   if(!(fp=fopen(cfg_groupsfile,"r")))
   {
      os_logwrite("(%s) Failed to read group configuration file %s",var->clientid,cfg_groupsfile);
      return(FALSE);
   }

   lastgroup=NULL;
   var->firstgroup=NULL;

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

         if(!(newgroup=(struct group *)malloc(sizeof(struct group))))
         {
            fclose(fp);
            return(FALSE);
         }

         newgroup->next=NULL;
         if(!var->firstgroup) var->firstgroup=newgroup;
         if(lastgroup) lastgroup->next=newgroup;
         lastgroup=newgroup;

         mystrncpy(newgroup->tagname,s,100);
         newgroup->group=s[c];
         mystrncpy(newgroup->aka,&s[d],40);
         mystrncpy(newgroup->jampath,&s[e],100);
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

