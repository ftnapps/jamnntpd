void mystrncpy(uchar *dest,uchar *src,long len);
void strip(uchar *str);
void makedate(time_t t,uchar *dest,uchar *tz);
bool setboolonoff(uchar *opt,bool *var);
bool getcfgword(uchar *line, ulong *pos, uchar *dest, ulong destlen);
bool matchgroup(uchar *groups,uchar group);
bool matchpattern(uchar *pat,uchar *ip);
void stripctrl(uchar *str);
bool matchcharset(uchar *pat,uchar *chrs,uchar *codepage);
ulong count8bit(uchar *text);


