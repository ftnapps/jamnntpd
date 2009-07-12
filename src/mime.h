void unmime(uchar *text,uchar *chrs,uchar *chrs2,ulong chrslen);
void setcharset(uchar *newchrs,uchar *chrs,uchar *chrs2,ulong chrslen);
void decodeqpheader(uchar *in,uchar *out);
void decodeqpbody(uchar *in,uchar *out);
void decodeb64(uchar *in,uchar *out);
void mimesendheaderline(struct var *var,uchar *keyword,uchar *data,uchar *chrs,uchar *fromaddr,bool noencode);
void mimemakeheaderline(uchar *dest,ulong destlen,uchar *keyword,uchar *data,uchar *chrs,uchar *fromaddr,bool noencode);


