#include "nntpserv.h"

/******************************* MIME decoding *****************************/

void decodeb64(uchar *in,uchar *out)
{
   uchar *found,*b64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   int c,d,count;
   ulong num;

   c=0;
   d=0;

   num=0;
   count=0;

   while(in[c])
   {
      if((found=strchr(b64,in[c])))
      {
         num <<= 6;
         num += found-b64;
         count++;
      }

      if(count == 4 || in[c] == '=')
      {
         if(count == 3) num <<= 6;
         if(count == 2) num <<= 12;

         if(count >= 2) out[d++] = (num >> 16) & 0xff;
         if(count >= 3) out[d++] = (num >> 8) & 0xff;
         if(count == 4) out[d++] = num & 0xff;

         count=0;
         num=0;
      }

      c++;
   }

   out[d]=0;
}


void decodeqpbody(uchar *in,uchar *out)
{
   int c,d;
   uchar *f1,*f2;
   uchar *hex="0123456789ABCDEF";

   /* Strip trailing whitespace */

   c=0;
   d=0;

   while(in[c])
   {
      if(in[c] == 13)
         while(d > 0 && (out[d-1] == 32 || out[d-1] == 9)) d--;

      out[d++]=in[c++];
   }

   /* Decode */

   c=0;
   d=0;

   while(out[c])
   {
      if(out[c] == '=' && out[c+1] == 13)
      {
         c+=2;
         if(out[c] == 10) c++;
      }
      else if(out[c] == '=' && out[c+1] && out[c+2])
      {
         f1=strchr(hex,out[c+1]);
         f2=strchr(hex,out[c+2]);

         if(!f1 || !f2)
            out[d++] = '?';

         else
            out[d++]=(f1-hex)*16+(f2-hex);

         c+=3;
      }
      else
      {
         out[d++] = out[c++];
      }
   }

   out[d]=0;
}

void decodeqpheader(uchar *in,uchar *out)
{
   int c,d;
   uchar *f1,*f2;
   uchar *hex="0123456789ABCDEF";

   c=0;
   d=0;

   while(in[c])
   {
      if(in[c] == '=' && in[c+1] && in[c+2])
      {
         f1=strchr(hex,in[c+1]);
         f2=strchr(hex,in[c+2]);

         if(!f1 || !f2)
            out[d++] = '?';

         else
            out[d++]=(f1-hex)*16+(f2-hex);

         c+=3;
      }
      else if(in[c] == '_')
      {
         out[d++] = ' ';
         c++;
      }
      else
      {
         out[d++] = in[c++];
      }
   }

   out[d]=0;
}


void setcharset(uchar *newchrs,uchar *chrs,uchar *chrs2,ulong chrslen)
{
   if(newchrs[0] != 0 && stricmp(chrs,newchrs) != 0)
   {
      /* We have a charset that doesn't match the previously found charset */

      if(chrs[0] == 0)
      {
         /* There was no previously found charset */

         mystrncpy(chrs,newchrs,chrslen);
      }
      else if(stricmp(chrs,"us-ascii") == 0)
      {
         /* Old charset was us-ascii. We assume that all other charsets are compatible with us-ascii.
            The charset found here will now be the charset for the message */

         mystrncpy(chrs,newchrs,chrslen);
      }
      else if(stricmp(newchrs,"us-ascii") != 0)
      {
         /* If the new charset isn't us-ascii, we have more than one charset in the message */

         mystrncpy(chrs2,newchrs,chrslen);
      }
   }
}

void unmime(uchar *text,uchar *chrs,uchar *chrs2,ulong chrslen)
{
   int c,d,begin,end;
   uchar *encoding;

   c=0;
   d=0;

   while(text[c])
   {
      if(text[c]=='=' && text[c+1]=='?')
      {
         /* Get charset */

         begin=c+2;
         end=begin;

         while(text[end] != '?' && text[end] != 0)
            end++;

         if(text[end] == 0)
         {
            text[d]=0;
            return; /* Ended prematurely */
         }

         text[end]=0;

         setcharset(&text[begin],chrs,chrs2,chrslen);

         /* Get encoding method */

         begin=end+1;
         end=begin;

         while(text[end] != '?' && text[end] != 0)
            end++;

         if(text[end] == 0)
         {
            text[d]=0;
            return; /* Ended prematurely */
         }

         text[end]=0;
         encoding=&text[begin];

         /* Get encoded data */

         begin=end+1;
         end=begin;

         while(text[end] != '?' && text[end] != 0)
            end++;

         if(text[end] == 0)
         {
            text[d]=0;
            return; /* Ended prematurely */
         }

         text[end]=0;

         /* Decode */

         if(stricmp(encoding,"q") == 0)
         {
            decodeqpheader(&text[begin],&text[d]);
            d=strlen(text);
         }
         else if(stricmp(encoding,"b") == 0)
         {
            decodeb64(&text[begin],&text[d]);
            d=strlen(text);
         }
         else
         {
            strcpy(&text[d],&text[begin]);
            d=strlen(text);
         }

         if(text[end+1] == '=') c=end+2;
         else                   c=end+1;

         /* Check if new encoded word follows directly */

         end=c;

         while(text[end] != 0 && (text[end] == 32 || text[end] == 9))
            end++;

         if(text[end] == '=' && text[end+1] == '?')
            c=end;
      }
      else
      {
         while(text[c] != 0 && text[c] != 32 && text[c] != 9)
            text[d++]=text[c++];

         if(text[c] != 0)
            text[d++]=text[c++];
      }
   }

   text[d]=0;
}

/********************************* MIME encoding *******************************/

/* Returns number of encoded characters */
int encodeqp(uchar *data,uchar *dest,uchar *specials,int maxdestlen)
{
   int c,d;
   uchar buf[10];

   c=0;
   d=0;

   while(data[c])
   {
      if(data[c] > 127 || strchr(specials,data[c]))
      {
         if(d + 3 > maxdestlen)
            return(c);

         sprintf(buf,"%02X",data[c]);

         dest[d++]='=';
         dest[d++]=buf[0];
         dest[d++]=buf[1];
         dest[d]=0;
      }
      else
      {
         if(d + 1 > maxdestlen)
            return(c);

         if(data[c] == ' ') dest[d++]='_';
         else               dest[d++]=data[c];

         dest[d]=0;
      }
      c++;
   }

   return(c);
}

/* Returns number of encoded characters */
int encodeb64(uchar *data,uchar *dest,uchar *specials,int maxdestlen)
{
   int c,d;
   ulong num;
   uchar *b64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

   c=0;
   d=0;

   while(data[c])
   {
      if(d + 4 > maxdestlen)
         return(c);

      if(data[c] && data[c+1] && data[c+2])
      {
         num=(data[c] << 16) + (data[c+1] << 8) + data[c+2];

         dest[d]   = b64[(num >> 18) & 63];
         dest[d+1] = b64[(num >> 12) & 63];
         dest[d+2] = b64[(num >> 6) & 63];
         dest[d+3] = b64[num & 63];
         dest[d+4] = 0;

         c+=3;
         d+=4;
      }
      else if(data[c] && data[c+1])
      {
         num=(data[c] << 16) + (data[c+1] << 8);

         dest[d]   = b64[(num >> 18) & 63];
         dest[d+1] = b64[(num >> 12) & 63];
         dest[d+2] = b64[(num >> 6) & 63];
         dest[d+3] = '=';
         dest[d+4] = 0;

         c+=2;
         d+=4;
      }
      else
      {
         num=(data[c] << 16);

         dest[d]   = b64[(num >> 18) & 63];
         dest[d+1] = b64[(num >> 12) & 63];
         dest[d+2] = '=';
         dest[d+3] = '=';
         dest[d+4] = 0;

         c+=1;
         d+=4;
      }
   }

   return(c);
}

void mimemakeheaderline(uchar *dest,ulong destlen,uchar *keyword,uchar *data,uchar *chrs,uchar *fromaddr,bool noencode)
{
   uchar *specials,*mimespecials="_=?\t",*fromspecials="_=?\t" "()<>@,;:\\\".[]";
   bool mime;
   ulong c,d,num8bit;
   uchar prefix[50],line[350],quoted[200],method;

   dest[0]=0;

   num8bit=count8bit(data);

   if(num8bit) mime=TRUE;
   else        mime=FALSE;

   if(num8bit > 5 && num8bit > strlen(data)/2) method='b';
   else                                        method='q';

   if(!mime || noencode)
   {
      if(fromaddr)
      {
         for(c=0;data[c];c++)
            if(strchr(fromspecials,data[c])) break;

         if(data[c])
         {
            /* Needs quoting */

            d=0;
            quoted[d++]='"';

            for(c=0;data[c];c++)
            {
               if(data[c] == '\\' || data[c] == 13)
                  quoted[d++]='\\';

               quoted[d++]=data[c];
            }

            quoted[d++]='"';
            quoted[d]=0;
         }
         else
         {
            strcpy(quoted,data);
         }

         sprintf(line,"%s: %s <%s>" CRLF,keyword,quoted,fromaddr);
      }
      else
      {
         sprintf(line,"%s: %s" CRLF,keyword,data);
      }

      if(strlen(line) < destlen)
         strcpy(dest,line);

      return;
   }

   if(fromaddr) specials=fromspecials;
   else         specials=mimespecials;

   sprintf(prefix,"=?%s?%c?",chrs,method);

   c=0;

   sprintf(line,"%s: %s",keyword,prefix);
   d=strlen(line);

   while(data[c])
   {
      if(method == 'b') c+=encodeb64(&data[c],&line[d],specials,76-d-2);
      if(method == 'q') c+=encodeqp(&data[c],&line[d],specials,76-d-2);

      if(data[c])
      {
         /* Start new line */

         strcat(line,"?=" CRLF);

         if(strlen(dest) + strlen(line) < destlen) strcat(dest,line);
         else return;

         sprintf(line," %s",prefix);
         d=strlen(line);
      }
   }

   strcat(line,"?=");

   if(fromaddr)
   {
      if(strlen(line) + strlen(fromaddr) + 3 > 76)
      {
         strcat(line,CRLF);

         if(strlen(dest) + strlen(line) < destlen) strcat(dest,line);
         else return;

         strcpy(line," ");
         d=strlen(line);
      }
      else
      {
         strcat(line," ");
      }

      strcat(line,"<");
      strcat(line,fromaddr);
      strcat(line,">");
   }

   strcat(line,CRLF);

   if(strlen(dest) + strlen(line) < destlen) strcat(dest,line);
}

void mimesendheaderline(struct var *var,uchar *keyword,uchar *data,uchar *chrs,uchar *fromaddr,bool noencode)
{
   uchar destbuf[1000];
   ulong c,lastline;
   uchar bak;

   /* Make buffer */

   mimemakeheaderline(destbuf,1000,keyword,data,chrs,fromaddr,noencode);

   /* Send buffer line by line */

   c=0;
   lastline=0;

   while(destbuf[c])
   {
      if(destbuf[c] == 13 && destbuf[c+1] == 10)
      {
         bak=destbuf[c+2];
         destbuf[c+2]=0;
         socksendtext(var,&destbuf[lastline]);
         destbuf[c+2]=bak;

         lastline=c+2;
         c+=2;
      }
      else
      {
         c++;
      }
   }
}


