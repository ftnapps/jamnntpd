#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#ifndef PLATFORM_WIN32
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

typedef int bool;

#include "jamlib/jam.h"

struct var
{
   struct sockio *sio;
   bool disconnect;

   struct group *currentgroup;
   ulong currentarticle;

   struct group *opengroup;
   s_JamBase *openmb;

   ulong inputpos;
   uchar *input;

   uchar clientid[100];

   struct group *firstgroup;

   uchar readgroups[50];
   uchar postgroups[50];

   uchar loginname[100];
   uchar password[100];

   bool opt_flowed;
   bool opt_showto;
};

#include "os.h"
#include "groups.h"
#include "misc.h"
#include "xlat.h"
#include "allow.h"
#include "sockio.h"
#include "login.h"

#define CR "\x0d"
#define LF "\x0a"

#define CRLF CR LF

#define SERVER_NAME       "JamNNTPd/" PLATFORM_NAME
#define SERVER_VERSION    "0.4"
#define SERVER_PIDVERSION "0.4"

#define SOCKIO_TIMEOUT 5*60

extern int server_openconnections;
extern int server_quit;

int get_server_openconnections(void);
int get_server_quit(void);

void server(SOCKET s);

#define CFG_PORT           5000
#define CFG_MAXCONN        5
#define CFG_DEBUG          FALSE

#define CFG_ALLOWFILE      CFG_BASEPATH "jamnntpd.allow"
#define CFG_GROUPSFILE     CFG_BASEPATH "jamnntpd.groups"
#define CFG_USERSFILE      CFG_BASEPATH "jamnntpd.users"

#define CFG_LOGFILE        LOG_BASEPATH "jamnntpd.log"

#define CFG_DEF_FLOWED     TRUE
#define CFG_DEF_SHOWTO     TRUE

extern ulong cfg_port;
extern ulong cfg_maxconn;
extern uchar *cfg_allowfile;
extern uchar *cfg_groupsfile;
extern uchar *cfg_logfile;
extern uchar *cfg_usersfile;

extern bool cfg_debug;
extern bool cfg_noxlat;
extern bool cfg_noecholog;
extern bool cfg_nostripre;
extern bool cfg_notearline;
extern bool cfg_noreplyaddr;

extern bool cfg_def_flowed;
extern bool cfg_def_showto;
