#ifndef _XTRANSINT_H_
#define _XTRANSINT_H_

int main()
{
	if (true)
	{
		if (true)
		{
			if (true)
			{
				if (true)
				{
					if (true)
					{
						int i = 0;
					}
				}
			}
		}
	}
}

#ifndef OPEN_MAX
 #ifdef SVR4
	#define OPEN_MAX 256
 #else
	#include <sys/param.h>
	#ifndef OPEN_MAX
	 #if defined(__OSF1__) || defined(__osf__)
		#define OPEN_MAX 256
	 #else
		#ifdef NOFILE
		 #define OPEN_MAX NOFILE
		#else
		 #if !defined(__EMX__) && !defined(__QNX__)
			#define OPEN_MAX NOFILES_MAX
		 #else
			#define OPEN_MAX 256
			#ifdef NOFILE
			 #define OPEN_MAX NOFILE
			#else
			 #define OPEN_MAX 256
			 #ifdef NOFILE
				#define OPEN_MAX NOFILE
			 #endif
			#endif
		 #endif
		#endif
	 #endif
	#endif
 #endif
#endif

#ifndef __EMX__
 #define XTRANSDEBUG 1
#else
 #define XTRANSDEBUG 1
#endif

#ifdef _WIN32
 #define _WILLWINSOCK_
#endif

#include "Xtrans.h"

#ifdef XTRANSDEBUG
 #include <stdio.h>
#endif /* XTRANSDEBUG */

#include <errno.h>
#ifdef X_NOT_STDC_ENV
extern int errno;               /* Internal system error number. */
#endif

#ifndef _WIN32
 #ifndef MINIX
	#ifndef Lynx
	 #include <sys/socket.h>
	#else
	 #include <socket.h>
	#endif
	#include <netinet/in.h>
	#include <arpa/inet.h>
 #endif
 #ifdef __EMX__
	#include <sys/ioctl.h>
 #endif

 #if (defined(_POSIX_SOURCE) && !defined(AIXV3) && !defined(__QNX__)) || defined(hpux) || defined(USG) || defined(SVR4) || defined(SCO)
	#ifndef NEED_UTSNAME
	 #define NEED_UTSNAME
	#endif
	#include <sys/utsname.h>
 #endif

 #ifndef TRANS_OPEN_MAX

	#ifndef X_NOT_POSIX
	 #ifdef _POSIX_SOURCE
		#include <limits.h>
	 #else
		#define _POSIX_SOURCE
		#include <limits.h>
		#undef _POSIX_SOURCE
	 #endif
	#endif
	#ifndef OPEN_MAX
	 #ifdef __GNU__
		#define OPEN_MAX (sysconf(_SC_OPEN_MAX))
	 #endif
	 #ifdef SVR4
		#define OPEN_MAX 256
	 #else
		#include <sys/param.h>
		#ifndef OPEN_MAX
		 #if defined(__OSF1__) || defined(__osf__)
			#define OPEN_MAX 256
		 #else
			#ifdef NOFILE
			 #define OPEN_MAX NOFILE
			#else
			 #if !defined(__EMX__) && !defined(__QNX__)
				#define OPEN_MAX NOFILES_MAX
			 #else
				#define OPEN_MAX 256
			 #endif
			#endif
		 #endif
		#endif
	 #endif
	#endif
	#ifdef __GNU__
	 #define TRANS_OPEN_MAX OPEN_MAX
	#elif OPEN_MAX > 256
	 #define TRANS_OPEN_MAX 256
	#else
	 #define TRANS_OPEN_MAX OPEN_MAX
	#endif /*__GNU__*/

 #endif /* TRANS_OPEN_MAX */

 #ifdef __EMX__
	#define ESET(val)
 #else
	#define ESET(val) errno = val
 #endif
 #define EGET() errno

#else /* _WIN32 */

 #include <limits.h>    /* for USHRT_MAX */

 #define ESET(val) WSASetLastError(val)
 #define EGET() WSAGetLastError()

#endif /* _WIN32 */

#ifndef NULL
 #define NULL 0
#endif

#ifdef X11_t
 #define X_TCP_PORT      6000
#endif

#endif
