#define SOCKIO_BUFLEN 1024

struct sockio
{
	SOCKET socket;
   uchar buf[SOCKIO_BUFLEN];
	int buflen,bufpos;
};
	
struct sockio *allocsockio(SOCKET socket);
void freesockio(struct sockio *sio);
int sockreadchar(struct sockio *sio);
bool sockreadline(struct var *var,uchar *buf,int len);
void socksendtext(struct var *var,uchar *buf);
void sockprintf(struct var *var,uchar *fmt,...);
