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

uchar *xlatstr(uchar *text,struct xlattab *xlattab);
bool readxlat(struct var *var);
void freexlat(struct var *var);
