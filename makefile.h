/* this makefile.h exports some paths known by
   Makefile. See chapter 20.5 of the autoconf
   manual (keyword: dedicated header file).
 */
#define xSTR(x) #x
#define Makefile__bindir     xSTR(/usr/bin)
#define Makefile__docdir     xSTR(/usr/share/doc/apl)
#define Makefile__sysconfdir xSTR(/etc)
#define Makefile__pkglibdir  xSTR(/usr/lib/apl)
#define Makefile__localedir  xSTR(/usr/share/locale)
#define Makefile__srcdir     xSTR(/home/eedjsa/apl-1.8/debian_tmp/apl-1.8)
#define Makefile__host_os    xSTR(linux-gnu)

#define CONFIGURE_ARGS       xSTR(./configure  '--build=x86_64-linux-gnu' '--prefix=/usr' '--includedir=${prefix}/include' '--mandir=${prefix}/share/man' '--infodir=${prefix}/share/info' '--sysconfdir=/etc' '--localstatedir=/var' '--disable-silent-rules' '--libexecdir=${prefix}/lib/apl' '--disable-maintainer-mode' '--disable-dependency-tracking' 'build_alias=x86_64-linux-gnu')

