#include "nntpserv.h"

uchar *xlatstr(uchar *text,struct xlattab *xlattab)
{
   uchar *newtext;
   long c,d,newlen;

   newlen=0;

   for(c=0;text[c];c++)
   {
      if(xlattab->table[text[c]*4])   newlen++;
      if(xlattab->table[text[c]*4+1]) newlen++;
      if(xlattab->table[text[c]*4+2]) newlen++;
      if(xlattab->table[text[c]*4+3]) newlen++;
   }

   newlen++; /* null-terminated */

   if(!(newtext=malloc(newlen)))
      return(NULL);

   d=0;

   for(c=0;text[c];c++)
   {
      if(xlattab->table[text[c]*4])   newtext[d++]=xlattab->table[text[c]*4];
      if(xlattab->table[text[c]*4+1]) newtext[d++]=xlattab->table[text[c]*4+1];
      if(xlattab->table[text[c]*4+2]) newtext[d++]=xlattab->table[text[c]*4+2];
      if(xlattab->table[text[c]*4+3]) newtext[d++]=xlattab->table[text[c]*4+3];
   }

   newtext[d]=0;

   return(newtext);
}

bool chsgetline(FILE *fp,uchar *str,ulong len)
{
	for(;;)
	{
		if(!fgets(str,len,fp))
			return(FALSE); /* EOF */

		if(str[0] != ';')
			return(TRUE); /* Not a comment */
	}
}

uchar chsgetbyte(struct var *var,uchar *filename,uchar *buf)
{
	ulong res;

   if(!buf)
		return(0);

	if(buf[0] == '\\')
	{
		if(buf[1] == '\\')
      {
         return('\\');
      }
		else if(buf[1] == '0')
      {
         return(0);
      }
		else if(buf[1] == 'd')
      {
         return (uchar)atoi(&buf[2]);
      }
		else if(buf[1] == 'x')
      {
	      sscanf(&buf[2],"%lx",&res);
 			return (uchar)res;
      }
		else
		{
			os_logwrite("(%s) Warning: Unknown byte string %s in %s",var->clientid,buf,filename);
			return(0);
		}
	}

   if(strlen(buf) > 1)
      os_logwrite("(%s) Warning: Too long byte string %s in %s",var->clientid,buf,filename);

	return(buf[0]);
}

bool chsgetword(uchar *line, ulong *pos, uchar *dest, ulong destlen)
{
   ulong begin;

   while(isspace(line[*pos]) && line[*pos]!=0)
      (*pos)++;

   if(line[*pos] == 0)
      return(FALSE);

   begin=*pos;

   while(line[*pos]!=0 && !isspace(line[*pos]))
      (*pos)++;

   if(line[*pos] != 0)
   {
      line[*pos]=0;
      (*pos)++;
   }

   mystrncpy(dest,&line[begin],destlen);

   return(TRUE);
}

struct xlattab *readchs(struct var *var,uchar *filename)
{
   FILE *fp;
	uchar buf[100];
   int level,c,basenum,readnum;
   ulong pos;
   struct xlattab *newxlattab,*lastxlattab;
   bool extended;

   uchar buf1[20],buf2[20],buf3[20],buf4[20],buf5[20];
   uchar ch1,ch2,ch3,ch4;
   bool res1,res2,res3,res4,res5;
   uchar table[1024];

   if(!(fp=fopen(filename,"r")))
   {
      os_logwrite("(%s) Warning: Could not open charset file %s, translation disabled",var->clientid,filename);
      return(NULL);
   }

	if(!chsgetline(fp,buf,100)) /* ID number */
	{
		os_logwrite("(%s) Warning: Unexpected EOF in %s when reading ID number, translation disabled",var->clientid,filename);
      fclose(fp);
      return(NULL);
	}

   if(atoi(buf) > 65535) extended=TRUE;
   else                  extended=FALSE;

	if(!chsgetline(fp,buf,100)) /* version number */
	{
		os_logwrite("(%s) Warning: Unexpected EOF in %s when reading version number, translation disabled",var->clientid,filename);
      fclose(fp);
      return(NULL);
	}

	if(!chsgetline(fp,buf,100)) /* level number */
	{
		os_logwrite("(%s) Warning: Unexpected EOF in %s when reading level number, translation disabled",var->clientid,filename);
      fclose(fp);
      return(NULL);
	}

	level=atoi(buf);

	if(!chsgetline(fp,buf,100))
	{
		os_logwrite("(%s) Warning: Unexpected EOF in %s when reading source charset, translation disabled",var->clientid,filename);
      fclose(fp);
      return(NULL);
	}

	if(!chsgetline(fp,buf,100))
	{
		os_logwrite("(%s) Warning: Unexpected EOF in %s when reading destination charset, translation disabled",var->clientid,filename);
      fclose(fp);
      return(NULL);
	}

	if(level !=1 && level != 2)
	{
		os_logwrite("(%s) Warning: %s is for level %d, translation disabled (only 1 and 2 are supported)",var->clientid,filename);
      fclose(fp);
      return(NULL);
	}

   /* init table */

   for(c=0;c<256;c++)
   {
      table[c*4]=c;
      table[c*4+1]=0;
      table[c*4+2]=0;
      table[c*4+3]=0;
   }

	/* read table */

   if(extended)
   {
      basenum=0;
      readnum=256;
   }
   else
   {
      readnum=128;

		if(level == 1) basenum=0;
		if(level == 2) basenum=128;
   }

   for(c=0;c<readnum;c++)
	{
      if(!chsgetline(fp,buf,100))
		{
		   os_logwrite("(%s) Warning: Unexpected EOF in %s when reading translation for %d, translation disabled",var->clientid,filename,basenum+c);
         fclose(fp);
         return(NULL);
		}

      if(strchr(buf,';'))
         *strchr(buf,';')=0;

      strip(buf);
      pos=0;

      res1=chsgetword(buf,&pos,buf1,20);
      res2=chsgetword(buf,&pos,buf2,20);
      res3=chsgetword(buf,&pos,buf3,20);
      res4=chsgetword(buf,&pos,buf4,20);
      res5=chsgetword(buf,&pos,buf5,20);

      ch1=0;
      ch2=0;
      ch3=0;
      ch4=0;

      if(res1) ch1=chsgetbyte(var,filename,buf1);
		if(res2) ch2=chsgetbyte(var,filename,buf2);
		if(res3) ch3=chsgetbyte(var,filename,buf3);
		if(res4) ch4=chsgetbyte(var,filename,buf4);

      if(res5)
         os_logwrite("(%s) Warning: %s has translations longer than four chars (char %d)",var->clientid,filename,c);

      table[(basenum+c)*4]=ch1;
      table[(basenum+c)*4+1]=ch2;
      table[(basenum+c)*4+2]=ch3;
      table[(basenum+c)*4+3]=ch4;
	}

   fclose(fp);

   if(!(newxlattab=malloc(sizeof(struct xlattab))))
   {
      return(FALSE);
   }

   lastxlattab=var->firstxlattab;

   if(lastxlattab)
   {
      while(lastxlattab->next)
         lastxlattab=lastxlattab->next;
   }
   
   newxlattab->next=NULL;
   if(!var->firstxlattab) var->firstxlattab=newxlattab;
   if(lastxlattab)        lastxlattab->next=newxlattab;

   mystrncpy(newxlattab->filename,filename,100);
   memcpy(newxlattab->table,table,sizeof(table));

   return(newxlattab);
}

bool matchcharset(uchar *pat,uchar *chrs,uchar *codepage)
{
   uchar buf[20],buf2[20];

   if(strchr(pat,','))
   {
      /* Match chrs and codepage */
      
      mystrncpy(buf,pat,20);
      
      if(strchr(buf,','))
         *strchr(buf,',')=0;
         
      mystrncpy(buf2,strchr(pat,',')+1,20);

      if(matchpattern(buf,chrs) && matchpattern(buf2,codepage))
         return(TRUE);

      return(FALSE);
   }
   else
   {
      /* Match chrs only */

      return matchpattern(pat,chrs);
   }
}

void setchrscodepage(uchar *chrs,uchar *codepage,uchar *str)
{
   if(strchr(str,','))
   {
      mystrncpy(chrs,str,20);
      
      if(strchr(chrs,','))
         *strchr(chrs,',')=0;

      mystrncpy(codepage,strchr(str,',')+1,20);
   }
   else
   {
      mystrncpy(chrs,str,20);
      codepage[0]=0;
   }
}

struct xlat *findreadxlat(struct var *var,struct group *group,uchar *ichrs,uchar *icodepage,uchar *destpat)
{
   uchar chrs[20],codepage[20];
   struct xlat *xlat;
   struct xlatalias *xlatalias;
   
   mystrncpy(chrs,ichrs,20);
   mystrncpy(codepage,icodepage,20);
   
   /* Do override */
   
   if(group->defaultchrs[0] == '!') 
      setchrscodepage(chrs,codepage,&group->defaultchrs[1]);
   
   else if(var->defaultreadchrs[0] == '!')
      setchrscodepage(chrs,codepage,&var->defaultreadchrs[1]);
   
   /* Set charset if missing */
   
   if(chrs[0] == 0 && group->defaultchrs[0] != 0 && group->defaultchrs[0] != '!') 
      setchrscodepage(chrs,codepage,group->defaultchrs);
      
   if(chrs[0] == 0 && var->defaultreadchrs[0] != 0 && var->defaultreadchrs[0] != '!')
      setchrscodepage(chrs,codepage,var->defaultreadchrs);      

   /* Replace if an alias */      
                  
   for(xlatalias=var->firstreadalias;xlatalias;xlatalias=xlatalias->next)
      if(matchcharset(xlatalias->pattern,chrs,codepage)) break;
      
   if(xlatalias)
      setchrscodepage(chrs,codepage,xlatalias->replace);
   
   /* Find in list */
      
   if(destpat)
   {
      for(xlat=var->firstreadxlat;xlat;xlat=xlat->next)
         if(matchcharset(xlat->fromchrs,chrs,codepage) && matchpattern(destpat,xlat->tochrs)) break;
   }
   else
   {
      for(xlat=var->firstreadxlat;xlat;xlat=xlat->next)
         if(matchcharset(xlat->fromchrs,chrs,codepage)) break;
   }
   
   return(xlat);
}

struct xlat *findpostxlat(struct var *var,uchar *ichrs,uchar *destpat)
{
   uchar chrs[20];
   struct xlat *xlat;
   struct xlatalias *xlatalias;
   
   mystrncpy(chrs,ichrs,20);
   
   /* Set charset if missing */
   
   if(chrs[0] == 0 && var->defaultpostchrs[0] != 0)
      mystrncpy(chrs,var->defaultpostchrs,20);
   
   /* Replace if an alias */      
                  
   for(xlatalias=var->firstpostalias;xlatalias;xlatalias=xlatalias->next)
      if(matchpattern(xlatalias->pattern,chrs)) break;
      
   if(xlatalias)
      mystrncpy(chrs,xlatalias->replace,20);
      
   /* Find in list */
      
   if(destpat)
   {
      for(xlat=var->firstpostxlat;xlat;xlat=xlat->next)
         if(matchpattern(xlat->fromchrs,chrs) && matchpattern(destpat,xlat->tochrs)) break;
   }
   else
   {
      for(xlat=var->firstpostxlat;xlat;xlat=xlat->next)
         if(matchpattern(xlat->fromchrs,chrs)) break;
   }
         
   return(xlat);
}

bool readxlat(struct var *var)
{
   FILE *fp;
   uchar s[1000],type[20],fromchrs[100],tochrs[100],option[100];
   uchar basename[100],fullfilename[250];
   bool res1,res2,res3;
   ulong pos,line;
   struct xlat *newxlat,*lastreadxlat,*lastpostxlat;
   struct xlatalias *newxlatalias, *lastreadalias,*lastpostalias;
   struct xlattab *xlattab;

   if(!(fp=fopen(cfg_xlatfile,"r")))
   {
      os_logwrite("(%s) Can't read xlat configuration file %s",var->clientid,cfg_xlatfile);
      return(FALSE);
   }

   lastreadxlat=NULL;
   lastpostxlat=NULL;
   lastreadalias=NULL;
   lastpostalias=NULL;

   var->firstreadxlat=NULL;
   var->firstpostxlat=NULL;
   var->firstreadalias=NULL;
   var->firstpostalias=NULL;
   var->firstxlattab=NULL;
   
   basename[0]=0;

   line=0;

   while(fgets(s,999,fp))
   {
      line++;
      strip(s);
      pos=0;

      if(s[0]!=0 && s[0]!='#')
      {
         res1=getcfgword(s,&pos,type,20);
         res2=getcfgword(s,&pos,fromchrs,100);
         res3=getcfgword(s,&pos,tochrs,100);

         if(stricmp(type,"chsdir")==0 && res2)
         {
            mystrncpy(basename,fromchrs,100);
         }
         else if(stricmp(type,"defaultpost")==0 && res2)
         {
            mystrncpy(var->defaultpostchrs,fromchrs,20);
         }
         else if(stricmp(type,"defaultread")==0 && res2)
         {
            mystrncpy(var->defaultreadchrs,fromchrs,20);
         }
         else if(stricmp(type,"readalias")==0 && res2 && res3)
         {
            if(!(newxlatalias=(struct xlatalias *)malloc(sizeof(struct xlatalias))))
            {
               fclose(fp);
               return(FALSE);
            }

            newxlatalias->next=NULL;

            if(!var->firstreadalias) var->firstreadalias=newxlatalias;
            if(lastreadalias) lastreadalias->next=newxlatalias;
            lastreadalias=newxlatalias;
         
            mystrncpy(newxlatalias->pattern,fromchrs,20);
            mystrncpy(newxlatalias->replace,tochrs,20);
         }
         else if(stricmp(type,"postalias")==0 && res2 && res3)
         {
            if(!(newxlatalias=(struct xlatalias *)malloc(sizeof(struct xlatalias))))
            {
               fclose(fp);
               return(FALSE);
            }

            newxlatalias->next=NULL;

            if(!var->firstpostalias) var->firstpostalias=newxlatalias;
            if(lastpostalias) lastpostalias->next=newxlatalias;
            lastpostalias=newxlatalias;
         
            mystrncpy(newxlatalias->pattern,fromchrs,20);
            mystrncpy(newxlatalias->replace,tochrs,20);
         }
         else if(stricmp(type,"post") == 0 || stricmp(type,"read") == 0)
         {
            if(!(newxlat=(struct xlat *)malloc(sizeof(struct xlat))))
            {
               fclose(fp);
               return(FALSE);
            }

            newxlat->next=NULL;

            if(stricmp(type,"post")==0)
            {
               if(!var->firstpostxlat) var->firstpostxlat=newxlat;
               if(lastpostxlat) lastpostxlat->next=newxlat;
               lastpostxlat=newxlat;
            }
            else
            {
               if(!var->firstreadxlat) var->firstreadxlat=newxlat;
               if(lastreadxlat) lastreadxlat->next=newxlat;
               lastreadxlat=newxlat;
            }

            mystrncpy(newxlat->fromchrs,fromchrs,20);
            mystrncpy(newxlat->tochrs,tochrs,20);

            newxlat->xlattab=NULL;
            newxlat->keepsoftcr=FALSE;
               
            while(getcfgword(s,&pos,option,100))
            {
               if(stricmp(option,"-keepsoftcr")==0)
               {
                  newxlat->keepsoftcr=TRUE;
               }
               else if(option[0] != '-' && !newxlat->xlattab)
               {
                  strcpy(fullfilename,basename);

                  if(strlen(fullfilename) != 0)
                  {
                     if(fullfilename[strlen(fullfilename)-1] != '/' && fullfilename[strlen(fullfilename)-1] != '\\')
                        strcat(fullfilename,"/");
                  }

                  strcat(fullfilename,option);

                  for(xlattab=var->firstxlattab;xlattab;xlattab=xlattab->next)
                     if(strcmp(xlattab->filename,fullfilename)==0) break;

                  if(xlattab)
                     newxlat->xlattab=xlattab;
                  
                  else
                     newxlat->xlattab=readchs(var,fullfilename);
               }
               else
               {
                  os_logwrite("(%s) Warning: Unknown option %s on line %lu in %s",var->clientid,option,line,cfg_xlatfile);
               }
            }
         }
         else
         {
            os_logwrite("(%s) Syntax error on line %lu in %s, skipping line",var->clientid,line,cfg_xlatfile);
         }
      }
   }

   fclose(fp);

   if(!var->firstpostxlat)
   {
      os_logwrite("(%s) No charsets for posting configured",var->clientid);
      return(FALSE);
   }

   return(TRUE);
}

void freexlat(struct var *var)
{
   freelist(var->firstreadxlat);
   freelist(var->firstpostxlat);
   freelist(var->firstreadalias);
   freelist(var->firstpostalias);
   freelist(var->firstxlattab);

   var->firstreadxlat=NULL;
   var->firstpostxlat=NULL;
   var->firstreadalias=NULL;
   var->firstpostalias=NULL;
   var->firstxlattab=NULL;
}

