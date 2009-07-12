#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define PLATFORM_NAME "2"

#define CFG_BASEPATH ""
#define LOG_BASEPATH ""

#define TRUE   1
#define FALSE  0

#define min(x,y) (x<y?x:y)

#define select os2_select
int os2_select(int n,fd_set *readfds,fd_set *writefds,fd_set *exceptfds,struct timeval *timeout);

