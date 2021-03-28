/* this makefile.h exports some paths known by
   Makefile. See chapter 20.5 of the autoconf
   manual (keyword: dedicated header file).
 */
#define xSTR(x) #x
#define Makefile__bindir     xSTR(/usr/local/bin)
#define Makefile__docdir     xSTR(/usr/local/share/doc/apl)
#define Makefile__sysconfdir xSTR(/usr/local/etc)
#define Makefile__pkglibdir  xSTR(/usr/local/lib/apl)
#define Makefile__localedir  xSTR(/usr/local/share/locale)
#define Makefile__srcdir     xSTR(/home/eedjsa/apl-1.8)
#define Makefile__host_os    xSTR(linux-gnu)

#define CONFIGURE_ARGS       xSTR(configure )
