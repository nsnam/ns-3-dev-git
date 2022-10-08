
#ifndef WIN32_INTERNET_H
#define WIN32_INTERNET_H

/* Winsock2.h is a cursed header that
 * causes multiple name collisions
 *
 * We use this private header to prevent them
 */

#include <winsock2.h>
#undef GetObject
#undef SetPort
#undef SendMessage
#undef CreateFile
#undef Rectangle
#undef interface

#endif // WIN32_INTERNET_H
