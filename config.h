/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* APserver listen port name */
#define APSERVER_PATH "/tmp/GNU-APL/APserver"

/* APserver TCP listen port */
#define APSERVER_PORT 16366

/* APserver transport: TCP=0, LINUX=1, UNIX=2 */
#define APSERVER_TRANSPORT 0

/* assert level */
#define ASSERT_LEVEL_WANTED 1

/* core count */
#define CORE_COUNT_WANTED 0

/* define to set options typical for software development */
/* #undef DEVELOP_WANTED */

/* define to enable dynamic logging */
/* #undef DYNAMIC_LOG_WANTED */

/* define to enable gprof profiling */
#define GPROF_WANTED -pg

/* Define to 1 iff pthread_setaffinity_np() is available */
#define HAVE_AFFINITY_NP 1

/* Define to 1 iff <curses.h> is present */
/* #undef HAVE_CURSES_H */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the <ext/atomicity.h> header file. */
#define HAVE_EXT_ATOMICITY_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <fftw3.h> header file. */
#define HAVE_FFTW3_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Define to 1 iff the OS has gettimeofday() */
#define HAVE_GETTIMEOFDAY 1

/* GTK+ version 3 NOT installed */
#define HAVE_GTK3 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `atomicity' library (-latomicity). */
/* #undef HAVE_LIBATOMICITY */

/* Define to 1 if you have the `c' library (-lc). */
#define HAVE_LIBC 1

/* Define to 1 if you have the `cairo' library (-lcairo). */
#define HAVE_LIBCAIRO 1

/* Define to 1 if you have the `curses' library (-lcurses). */
#define HAVE_LIBCURSES 1

/* Define to 1 if you have the `dl' library (-ldl). */
#define HAVE_LIBDL 1

/* Define to 1 if you have the `execinfo' library (-lexecinfo). */
/* #undef HAVE_LIBEXECINFO */

/* Define to 1 if you have the `fftw3' library (-lfftw3). */
#define HAVE_LIBFFTW3 1

/* Define to 1 if you have the `gdk-3' library (-lgdk-3). */
#define HAVE_LIBGDK_3 1

/* Define to 1 if you have the `gtk-3' library (-lgtk-3). */
#define HAVE_LIBGTK_3 1

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `ncurses' library (-lncurses). */
#define HAVE_LIBNCURSES 1

/* Define to 1 if you have the `nsl' library (-lnsl). */
#define HAVE_LIBNSL 1

/* Define to 1 if you have the `pcre2-32' library (-lpcre2-32). */
#define HAVE_LIBPCRE2_32 1

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the `tinfo' library (-ltinfo). */
#define HAVE_LIBTINFO 1

/* Define to 1 if you have the `X11' library (-lX11). */
#define HAVE_LIBX11 1

/* Define to 1 if you have the `X11-xcb' library (-lX11-xcb). */
#define HAVE_LIBX11_XCB 1

/* Define to 1 if you have the `xcb' library (-lxcb). */
#define HAVE_LIBXCB 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1

/* Define to 1 if you have the `munmap' function. */
#define HAVE_MUNMAP 1

/* Define to 1 iff <ncurses.h> is present */
#define HAVE_NCURSES_H 1

/* Define to 1 iff OSAtomicAdd32Barrier() is available */
/* #undef HAVE_OSX_ATOMIC */

/* Define to 1 if PostgreSQL libraries are available */
#define HAVE_POSTGRESQL 1

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* Define to 1 iff pthread_setname_np() is available */
#define HAVE_PTHREAD_SETNAME_NP 1

/* Define to 1 iff the CPU has rdtsc opcode */
#define HAVE_RDTSC 1

/* Define to 1 iff sem_init() works */
#define HAVE_SEM_INIT 1

/* Define to 1 iff atomic_add_32_nv() is available */
/* #undef HAVE_SOLARIS_ATOMIC */

/* Have the SQLITE3 library */
#define HAVE_SQLITE3 /**/

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if stdbool.h conforms to C99. */
/* #undef HAVE_STDBOOL_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 iff <term.h> is present */
#define HAVE_TERM_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <utmpx.h> header file. */
#define HAVE_UTMPX_H 1

/* Define to 1 if you have the <X11/Xlib.h> header file. */
#define HAVE_X11_XLIB_H 1

/* Define to 1 if you have the <X11/Xlib-xcb.h> header file. */
#define HAVE_X11_XLIB_XCB_H 1

/* Define to 1 if you have the <X11/Xutil.h> header file. */
#define HAVE_X11_XUTIL_H 1

/* Define to 1 if you have the <xcb/xcb.h> header file. */
#define HAVE_XCB_XCB_H 1

/* Define to 1 if the system has the type `_Bool'. */
/* #undef HAVE__BOOL */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* max. rank of APL values */
#define MAX_RANK_WANTED 8

/* Name of package */
#define PACKAGE "apl"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "bug-apl@gnu.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "GNU APL"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "GNU APL 1.8"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "apl"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.gnu.org/software/apl/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.8"

/* define to enable performance counters */
/* #undef PERFORMANCE_COUNTERS_WANTED */

/* define to have support for rational numbers (EXPERIMENTAL!) */
/* #undef RATIONAL_NUMBERS_WANTED */

/* ./configure with --with-postgresql */
/* #undef REALLY_WANT_PostgreSQL */

/* ./configure with --with-sqlite3 */
/* #undef REALLY_WANT_SQLITE3 */

/* security level */
#define SECURITY_LEVEL_WANTED 0

/* short value cellcount */
#define SHORT_VALUE_LENGTH_WANTED 12

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* PostgreSQL code compiles */
#define USABLE_PostgreSQL 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* define to enable CHECK macro */
/* #undef VALUE_CHECK_WANTED */

/* define to enable value events */
/* #undef VALUE_HISTORY_WANTED */

/* Version number of package */
#define VERSION "1.8"

/* define to enable tracing of value flags */
/* #undef VF_TRACING_WANTED */

/* define to have visible markers (DONT!) */
/* #undef VISIBLE_MARKERS_WANTED */

/* Define if compiling for Android */
/* #undef WANT_ANDROID */

/* Define if building the Erlang interface */
/* #undef WANT_ERLANG */

/* Define if building libapl.so */
/* #undef WANT_LIBAPL */

/* Define if building lib_gnu_apl.so */
/* #undef WANT_PYTHON */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT32_T */

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT8_T */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to the type of a signed integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int32_t */

/* Define to the type of a signed integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int64_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to the type of an unsigned integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint32_t */

/* Define to the type of an unsigned integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint64_t */

/* Define to the type of an unsigned integer type of width exactly 8 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint8_t */
