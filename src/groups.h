struct group
{
   struct group *next;
   uchar tagname[100];
   uchar group;
   uchar jampath[100];
   uchar aka[40];
   uchar defaultchrs[20];
   bool nochrs;
};

bool readgroups(struct var *var);
void freegroups(struct var *var);

