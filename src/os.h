#ifdef PLATFORM_WIN32
#include "os_win32.h"
#endif

#ifdef PLATFORM_LINUX
#include "os_linux.h"
#endif

bool os_init(void);
void os_free(void);
void os_startserver(void (*srv)(SOCKET sock),SOCKET sock);
void os_sleep(int x);
void os_getexclusive(void);
void os_stopexclusive(void);
void os_logwrite(uchar *fmt,...);
void os_showerror(uchar *fmt,...);
int os_errno(void);    
uchar *os_strerr(int err,uchar *str,ulong len);
