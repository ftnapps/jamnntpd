struct xlattab
{
   struct xlattab *next;
   uchar filename[100];
   uchar table[1024];
};

struct xlat
{
   struct xlat *next;
   uchar fromchrs[20];
   uchar tochrs[20];
   struct xlattab *xlattab;
};

struct xlatalias
{
   struct xlatalias *next;
   uchar pattern[20];
   uchar replace[20];
};

uchar *xlatstr(uchar *text,struct xlattab *xlattab);
bool readxlat(struct var *var);
void freexlat(struct var *var);
bool matchcharset(uchar *pat,uchar *chrs,uchar *codepage);
void setchrscodepage(uchar *chrs,uchar *codepage,uchar *str);
struct xlat *findreadxlat(struct var *var,struct group *group,uchar *ichrs,uchar *icodepage,uchar *destpat);
struct xlat *findpostxlat(struct var *var,uchar *ichrs);
