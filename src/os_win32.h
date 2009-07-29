#include <winsock.h>
#include <process.h>

#define PLATFORM_NAME "Win32"

#define CFG_BASEPATH ""
#define LOG_BASEPATH ""

#undef close
#define close closesocket


