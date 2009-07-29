#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define stricmp strcasecmp
#define strnicmp strncasecmp

#define PLATFORM_NAME "Linux"

#define CFG_BASEPATH "/etc/"
#define LOG_BASEPATH "/var/spool/log/"

#define TRUE 1
#define FALSE 0

#define min(x,y) (x<y?x:y)
