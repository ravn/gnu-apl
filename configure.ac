#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# vim: et:ts=4:sw=4

AC_PREREQ([2.65])
AC_INIT([GNU APL], [1.8], [bug-apl@gnu.org])
configure_options="$0 $*"
AC_PREFIX_DEFAULT( /usr/local )
AC_CONFIG_AUX_DIR([.])
AC_USE_SYSTEM_EXTENSIONS

# always enable dependency tracking regardless of the user's choice.
# This avoids stale .o files after SVN updates. enable_dependency_tracking
# needs to be set early in order to be recognised!
#
# enable_dependency_tracking=no

AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE([disable])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

# use g++ for tests
#
AC_LANG(C++)

LT_INIT([dlopen])

# Checks for programs.
#
AC_PROG_CXX
AC_PROG_CC

AC_GNU_SOURCE

# add the current directory (i.e. from where ./configure was called) to the
# include path do that the files created by ./configure are found. This
# is (only) needed for VPATH builds since otherwise the directory from which
# ./configure is run is the same as the directory where ./configure places
# its output files (i.e. directory '.').
#
CXXFLAGS="$CXXFLAGS -I $(pwd)"

##############################################################################
# Checks for header files.
#

# check for ncurses.h first and for curses.h if ncurses.h was not found

AC_CHECK_HEADER(
    [ncurses.h],
    [AC_DEFINE_UNQUOTED( [HAVE_NCURSES_H], [1],
                         [Define to 1 iff <ncurses.h> is present])],
    [AC_CHECK_HEADER(
        [curses.h],
        [AC_DEFINE_UNQUOTED( [HAVE_CURSES_H], [1],
                             [Define to 1 iff <curses.h> is present])])
    ])

AC_CHECK_HEADER(
    [term.h],
    [AC_DEFINE_UNQUOTED( [HAVE_TERM_H], [1],
                         [Define to 1 iff <term.h> is present])],
    [],
    [ [ # if HAVE_NCURSES_H   ]
      [ # include <ncurses.h> ]
      [ # elif HAVE_CURSES_H  ]
      [ # include <curses.h>  ]
      [ # endif ]
    ])

AC_CHECK_HEADERS([  \
    dirent.h        \
    execinfo.h      \
    fcntl.h         \
    fftw3.h         \
    inttypes.h      \
    stdint.h        \
    stdlib.h        \
    string.h        \
    strings.h       \
    unistd.h        \
    sys/un.h        \
    ext/atomicity.h \
    xcb/xcb.h       \
    X11/Xlib.h      \
    X11/Xlib-xcb.h  \
    X11/Xutil.h     \
    utmpx.h])

# xcb/xproto.h needs xcb/xcb.h
AC_CHECK_HEADER(xcb/xproto.h, [], [], [#include <xcb/xcb.h>])

# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_DEFINE_UNQUOTED([CONFIGURE_ARGS],
                   "$0 $ac_configure_args",
                   [how ./configure was called])

##############################################################################
#
# Various checks for library functions. We use inverse alphabetic order although
# the order should be least-dependent to most-dependent (or vice versa?). The
# checks that come earlier in this file come later on the linker command line.
#
# That is, if e.g. lib_A requires lib_B, then the linker command line should be
# ... lib_A ... lib_B   and the test for lib_B should come before the test for
# lib_A in this file.
#
##############################################################################

AC_FUNC_ERROR_AT_LINE
AC_FUNC_MMAP
AC_CHECK_FUNCS([floor memset munmap pow sqrt strchr strerror])

AC_CHECK_LIB([X11-xcb],   [XGetXCBConnection])
AC_CHECK_LIB([X11],       [XOpenDisplay])
AC_CHECK_LIB([xcb],       [xcb_connect])
AC_CHECK_LIB([tinfo],     [tparm])
AC_CHECK_LIB([socket],    [socket])   # for Solaris
AC_CHECK_LIB([pthread],   [sem_init])
AC_CHECK_LIB([nsl],       [yp_all])   # for Solaris
AC_CHECK_LIB([curses],    [setupterm])
AC_CHECK_LIB([ncurses],   [setupterm])
AC_CHECK_LIB([m],         [acosh])
AC_CHECK_LIB([gtk-3],     [gtk_init])
AC_CHECK_LIB([gdk-3],     [gdk_init])
AC_CHECK_LIB([fftw3],     [fftw_plan_dft])
AC_CHECK_LIB([execinfo],  [backtrace])
AC_CHECK_LIB([dl],        [dlopen])
AC_CHECK_LIB([cairo],     [cairo_fill])
AC_CHECK_LIB([c],         [initstate_r])
AC_CHECK_LIB([atomicity], [__atomic_fetch_add_4])

# check if SQLite3 is wanted and installed
m4_include([m4/ax_lib_sqlite3.m4])            AX_LIB_SQLITE3([])
if test "x$WANT1_SQLITE3" = "xyes"; then
   AC_DEFINE_UNQUOTED([REALLY_WANT_SQLITE3], [1],
                      [./configure with --with-sqlite3])
fi


# check if PostgreSQL is wanted and installed
m4_include([m4/ax_lib_postgresql.m4])         AX_LIB_POSTGRESQL([])
if test "x$want1_postgresql" = "xyes"; then
   AC_DEFINE_UNQUOTED([REALLY_WANT_PostgreSQL], [1],
                      [./configure with --with-postgresql])
fi

# check if PostgreSQL is usable (if present)
if test "x$found_postgresql" = "xyes" ; then
    AC_MSG_CHECKING([for PostgreSQL usability])
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $POSTGRESQL_CFLAGS"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <libpq-fe.h>]],
                      [[PGconn *db = PQconnectdbParams( 0, 0, 1 );]])],
                      [USABLE_PostgreSQL=yes],
                      [USABLE_PostgreSQL=no])

    CPPFLAGS="$saved_CPPFLAGS"
    AC_MSG_RESULT([$USABLE_PostgreSQL])
    if test "x$USABLE_PostgreSQL" = "xyes"; then
        AC_DEFINE_UNQUOTED([USABLE_PostgreSQL], [1], [PostgreSQL code compiles])
    fi
fi

# check if libpcre is installed
m4_include([m4/ax_path_lib_pcre.m4])
   AX_PATH_LIB_PCRE([])
AM_CONDITIONAL(HAVE_LIBPCRE2_32,
               test "x$ac_cv_lib_pcre2_32_pcre2_compile_32" = xyes)

################################# GTK3 check #################################
#
AC_ARG_WITH([gtk3],
    AS_HELP_STRING([--with-gtk3],
    [enable ???GTK (interface to GTK+ version 3; needs libgtk-3 and more)]))

HAVE_gtk3=no
AC_SUBST(GTK_LDFLAGS)
AC_SUBST(GTK_CFLAGS)
GTK_LDFLAGS=
GTK_CFLAGS=
AC_MSG_CHECKING([whether support for ???GTK (needs GTK version 3) is desired])
if test "x$with_gtk3" = "xno"       # user has explicitly disabled GTK
then
    AC_MSG_RESULT([no - (user has explicitly disabled ???GTK)])
else
    # user wants ???GTK)
    AC_MSG_RESULT([yes])

    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)  # locate pkg-config
    if test x$PKG_CONFIG != xno
    then
        AC_MSG_CHECKING([if package gtk+-3.0 is installed])
        if $PKG_CONFIG --exists gtk+-3.0
        then
            AC_MSG_RESULT([yes])
            GTK_CFLAGS=`$PKG_CONFIG --cflags gtk+-3.0`
            GTK_LDFLAGS=`$PKG_CONFIG --libs gtk+-3.0`
            HAVE_gtk3=yes
        else
            AC_MSG_RESULT([no])
            HAVE_gtk3=no
        fi
    else
        AC_MSG_RESULT([no (cannot use GTK without pkg-config)])
        HAVE_gtk3=no
    fi
fi

# make HAVE_GTK3 visible in Makefile.am
AM_CONDITIONAL(HAVE_GTK3, test "x$HAVE_gtk3" = xyes)

# make HAVE_GTK3 visible im config.h
if test "x$HAVE_gtk3" = xyes; then
   AC_DEFINE_UNQUOTED(HAVE_GTK3, [1], [GTK+ version 3 installed])
else
   AC_DEFINE_UNQUOTED(HAVE_GTK3, [0], [GTK+ version 3 NOT installed])
fi

################################# rdtsc check ################################
# check if rdtsc (read CPU cycle counter is available.
# This is expected to succeed only on Intel CPUs
#
AC_MSG_CHECKING([whether CPU has rdtsc (read CPU cycle counter) opcode])
AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[]],
        [[unsigned int lo, hi;
          __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));]]
    )], [HAVE_rdtsc=yes], [HAVE_rdtsc=no])
if test "x$HAVE_rdtsc" = "xyes"; then
   AC_DEFINE_UNQUOTED([HAVE_RDTSC], [1],
                      [Define to 1 iff the CPU has rdtsc opcode])
fi
AM_CONDITIONAL(HAVE_RDTSC, test "x$HAVE_rdtsc" = xyes)
AC_MSG_RESULT([$HAVE_rdtsc])

################################# gettimeofday check #########################
# check if gettimeofday() counter is available on the OS
#
AC_MSG_CHECKING([whether gettimeofday exists])
AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <sys/time.h>]],
        [[timeval tv; gettimeofday(&tv, 0); ]]
    )], [HAVE_gettimeofday=yes], [HAVE_gettimeofday=no])
if test "x$HAVE_gettimeofday" = "xyes"; then
   AC_DEFINE_UNQUOTED([HAVE_GETTIMEOFDAY], [1],
                      [Define to 1 iff the OS has gettimeofday()])
fi
AM_CONDITIONAL(HAVE_RDTSC, test "x$HAVE_rdtsc" = xyes)
AC_MSG_RESULT([$HAVE_rdtsc])

############################### sem_init() check ##############################
# check if sem_init() works (it does not on OS-X)
#
apl_save_CXXFLAGS=$CXXFLAGS
CXXFLAGS="-Wall -Werror"

# This is expected only on Intel CPUs
AC_MSG_CHECKING([whether sem_init() works])
AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <semaphore.h>]],
        [[sem_t sema;
          sem_init(&sema, 0, 0);
        ]]
    )], [HAVE_SEM_INIT=yes], [HAVE_SEM_INIT=no])
if test "x$HAVE_SEM_INIT" = "xyes"; then
   AC_DEFINE_UNQUOTED([HAVE_SEM_INIT], [1], [Define to 1 iff sem_init() works])
fi
AM_CONDITIONAL(HAVE_SEM_INIT, test "x$HAVE_SEM_INIT" = xyes)
CXXFLAGS=$apl_save_CXXFLAGS
AC_MSG_RESULT([$HAVE_SEM_INIT])


######################## pthread_getaffinity_np() check #######################
# see if pthread_getaffinity_np() and pthread_setaffinity_np are available
#
AC_MSG_CHECKING([whether pthread_setaffinity_np is available])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([ [#include <pthread.h>]
                       [#include <ext/atomicity.h>]
                     ],
                     [ [cpu_set_t cpuset;]
                       [pthread_setaffinity_np(0, sizeof(cpuset), &cpuset);]
                       [pthread_getaffinity_np(0, sizeof(cpuset), &cpuset);]
                       [_Atomic_word count = 0;]
                       [__gnu_cxx::__exchange_and_add_dispatch(&count, 1);]
                       [__gnu_cxx::__atomic_add_dispatch(&count, 1);]
                     ])],
    [HAVE_AFFINITY_NP=yes
     AC_DEFINE_UNQUOTED([HAVE_AFFINITY_NP], [1],
                        [Define to 1 iff pthread_setaffinity_np() is available])
    ],
    [HAVE_AFFINITY_NP=no])
AM_CONDITIONAL(HAVE_AFFINITY_NP, test "x$HAVE_AFFINITY_NP" = xyes)
AC_MSG_RESULT([$HAVE_AFFINITY_NP])

######################## pthread_setname_np() check #######################
# see if pthread_setname_np is available
#
AC_MSG_CHECKING([whether pthread_setname_np is available])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([ [#include <pthread.h>]
                     ],
                     [ [pthread_t thread = 0;]
                       [pthread_setname_np(thread, "thread_name");]
                     ])],
    [HAVE_PTHREAD_SETNAME_NP=yes
     AC_DEFINE_UNQUOTED([HAVE_PTHREAD_SETNAME_NP], [1],
                        [Define to 1 iff pthread_setname_np() is available])
    ],
    [HAVE_PTHREAD_SETNAME_NP=no])
AC_MSG_RESULT([$HAVE_PTHREAD_SETNAME_NP])

############################### atomicity checks ##############################
# check for alternatives for ext/atomicity.h on OS/X 
#
AC_MSG_CHECKING([whether OSAtomicAdd32Barrier() is available])
apl_save_CXXFLAGS=$CXXFLAGS
CXXFLAGS="-Wall -Werror"
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([ [#include <libkern/OSAtomic.h>] ],
                     [ [OSAtomicAdd32Barrier(0, 0);]   ])],
    [HAVE_OSX_ATOMIC=yes
     AC_DEFINE_UNQUOTED([HAVE_OSX_ATOMIC], [1],
                          [Define to 1 iff OSAtomicAdd32Barrier() is available])
    ],
    [HAVE_OSX_ATOMIC=no])
CXXFLAGS=$apl_save_CXXFLAGS
AC_MSG_RESULT([$HAVE_OSX_ATOMIC])


# check for alternatives for ext/atomicity.h on Solaris
#
AC_MSG_CHECKING([whether atomic_add_32_nv() is available])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([ [#include <atomic.h>] ],
                     [ [atomic_add_32_nv(0, 0);]
                       [atomic_add_32   (0, 0);]
                     ])],
    [HAVE_SOLARIS_ATOMIC=yes
     AC_DEFINE_UNQUOTED([HAVE_SOLARIS_ATOMIC], [1],
                          [Define to 1 iff atomic_add_32_nv() is available])
    ],
    [HAVE_SOLARIS_ATOMIC=no])
AC_MSG_RESULT([$HAVE_SOLARIS_ATOMIC])


echo
echo "   *** decisions made by the user ... ***"
echo

AC_MSG_CHECKING([if user wants ctrl-d to be del-char (moving EOT to ctrl-z)])
user_wants_ctrld_del=no
AC_ARG_WITH( [ctrld_del],
             [AS_HELP_STRING([--with-ctrld_del],
                             [enable to use ^D as del-char, ^Z as EOT])],
             [user_wants_ctrld_del=$withval])

if test "x$user_wants_ctrld_del" = xyes ; then
    AC_DEFINE([WANT_CTRLD_DEL], [1],
              [Define to make ^D a del-char and ^Z be EOT])
fi

AM_CONDITIONAL(WANT_CTRLD_DEL, test "x$user_wants_ctrld_del" = xyes)
AC_MSG_RESULT([$user_wants_ctrld_del])

# check if the user compiles for ANDROID
#
AC_MSG_CHECKING([if we are compiling for Android])
user_wants_android=no
AC_ARG_WITH( [android],
             [AS_HELP_STRING([--with-android],
                             [enable when compiling for Android])],
             [user_wants_android=$withval])

if test "x$user_wants_android" = xyes ; then
   AC_DEFINE([WANT_ANDROID], [1], [Define if compiling for Android])
fi
AM_CONDITIONAL(WANT_ANDROID, test "x$user_wants_android" = xyes)
AC_MSG_RESULT([$user_wants_android])


# check if the user wants the Erlang interface (and therefore also libapl)
#
AC_MSG_CHECKING([if we want to build an erlang interface (implies libapl.so)])
user_wants_erlang=no
AC_ARG_WITH( [erlang],
             [AS_HELP_STRING([--with-erlang],
                             [enable to build the Erlang interface])],
             [user_wants_erlang=$withval])

if test "x$user_wants_libapl" = xyes ; then
   AC_DEFINE([WANT_ERLANG], [1], [Define if building the Erlang interface])
fi

AM_CONDITIONAL(WANT_ERLANG, test "x$user_wants_erlang" = xyes)
AC_MSG_RESULT([$user_wants_erlang])

# check if the user wants to build libapl.so
#
AC_MSG_CHECKING([if we want to build libapl.so])
user_wants_libapl=$user_wants_erlang   # Erlang needs libapl
AC_ARG_WITH( [libapl],
             [AS_HELP_STRING([--with-libapl],
                             [enable to build libapl.so])],
             [user_wants_libapl=$withval])

if test "x$user_wants_libapl" = xyes ; then
   AC_DEFINE([WANT_LIBAPL], [1], [Define if building libapl.so])
fi

AM_CONDITIONAL(WANT_LIBAPL, test "x$user_wants_libapl" = xyes)
AC_MSG_RESULT([$user_wants_libapl])

# check if the user wants to build the python extension gnu_apl.so
#
AC_MSG_CHECKING([if we want to build libpython_apl.so])
user_wants_python=no
AC_SUBST(PYTHON_CFLAGS)
PYTHON_CFLAGS=
AC_ARG_WITH( [python],
             [AS_HELP_STRING([--with-python],
             [enable to build python extension lib_gnu_apl.so])],
             [user_wants_python=$withval])

if test "$user_wants_python" = yes ; then
   AC_DEFINE([WANT_PYTHON], [1], [Define if building lib_gnu_apl.so])
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)  # locate pkg-config
    if test x$PKG_CONFIG != xno ; then        # we have pkg-config
       if $PKG_CONFIG --exists python ; then  # and pkg-config knows python
            PYTHON_CFLAGS=$( $PKG_CONFIG --cflags python3 )
       fi
   fi
fi
AM_CONDITIONAL(WANT_PYTHON, test "x$user_wants_python" = xyes)
AC_MSG_RESULT([$user_wants_python])

AC_CONFIG_FILES([Makefile
                 build/Makefile
                 debian/Makefile
                 debian/source/Makefile
                 doc/Makefile
                 doc/DALY/Makefile
                 erlang/Makefile
                 erlang/apl.erl
                 gnu-apl.d/Makefile
                 gnu-apl.d/preferences
                 HOWTOs/Makefile
                 rpm/Makefile
                 rpm/apl.spec
                 src/Makefile
                 src/APs/Makefile
                 src/Gtk/Makefile
                 src/emacs_mode/Makefile
                 src/native/Makefile
                 src/sql/Makefile
                 src/testcases/Makefile
                 src/workspaces/Makefile
                 support-files/Makefile
                 support-files/105-key-International-Keyboard/Makefile
                 support-files/Bill_Daly_Ubuntu/Makefile
                 support-files/Dirk/Makefile
                 support-files/Dyalog-Keyboard/Makefile
                 support-files/Juergen-Mint-19/Makefile
                 support-files/old-Keyboard/Makefile
                 support-files/OS-X-Keyboard/Makefile
                 support-files/Unicomp-Keyboard/Makefile
                 support-files/WASD-Keyboard/Makefile
                 tools/Makefile
                 websock/Makefile
                 websock/client/Makefile
                 websock/server/Makefile
                 workspaces/Makefile
                 wslib3/Makefile
                 wslib3/DALY/Makefile
                 wslib4/Makefile
                 wslib5/Makefile
                 wslib5/APLComponentFiles/Makefile
                 wslib5/DALY/Makefile
                 wslib5/iso-apl-cf/Makefile
                ])

# AM_CXXFLAGS: do/dont -Werror ?
AC_ARG_VAR(CXX_WERROR,
   [ turn all warnings into errors (compiler flag -Werror) default: no) ])
if test "x$CXX_WERROR" = "xyes"; then CXX_WERROR="-Werror";
                                 else CXX_WERROR=""; fi

# APL: development configuration desired ? Set this before other xxx_WANTED
# options so that the implied xxx_WANTED options set here can be overridden
# further down
AC_ARG_VAR(DEVELOP_WANTED,
   [ set development options (see README-2-configure) default: no])
if test "x$DEVELOP_WANTED" = "xyes"; then
   AC_DEFINE_UNQUOTED([DEVELOP_WANTED], [yes],
                   [define to set options typical for software development])
   ASSERT_LEVEL_WANTED=2
   DYNAMIC_LOG_WANTED=yes
   PERFORMANCE_COUNTERS_WANTED=yes
   RATIONAL_NUMBERS_WANTED=yes
   VALUE_CHECK_WANTED=yes
   VALUE_HISTORY_WANTED=yes
fi

# APL: max. rank ?
AC_ARG_VAR(MAX_RANK_WANTED,
   [ the max. Rank of APL values (see README-2-configure) default: 8) ])
if test "x$MAX_RANK_WANTED" = "x"; then MAX_RANK_WANTED=8; fi
AC_DEFINE_UNQUOTED([MAX_RANK_WANTED], [$MAX_RANK_WANTED],
                   [max. rank of APL values ])


# APL: CHECK() macro desired ?
AC_ARG_VAR(VALUE_CHECK_WANTED,
   [ enable CHECK() macro (see README-2-configure) default: no ])
if test "x$VALUE_CHECK_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VALUE_CHECK_WANTED], [yes],
                   [ define to enable CHECK macro ])
fi


# APL: performance counters desired ?
AC_MSG_CHECKING([whether internal performance counters shall be enabled)])
AC_ARG_VAR(PERFORMANCE_COUNTERS_WANTED,
   [ enable performance counters (see README-2-configure) default: no ])
if test "x$PERFORMANCE_COUNTERS_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([PERFORMANCE_COUNTERS_WANTED], [yes],
                   [ define to enable performance counters ])
else PERFORMANCE_COUNTERS_WANTED="no"
fi
AC_MSG_RESULT([$PERFORMANCE_COUNTERS_WANTED])


# APL: ADD_EVENT() macro and friends desired ?
AC_MSG_CHECKING([whether to remember the history of APL values (debug feature)])
AC_ARG_VAR(VALUE_HISTORY_WANTED,
   [ enable value events (a debug facility, see README-2-configure) default: no ])
if test "x$VALUE_HISTORY_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VALUE_HISTORY_WANTED], [yes],
                   [ define to enable value events])
else VALUE_HISTORY_WANTED="no"
fi
AC_MSG_RESULT([$VALUE_HISTORY_WANTED])


# APL: gprof profiling ?
AC_MSG_CHECKING([whether gprof profiling shall be enabled])
AC_ARG_VAR(GPROF_WANTED,
   [ enable gprof (-pg) (see README-2-configure) default: no ])
if test "x$GPROF_WANTED" = "xyes"; then
   GPROF_WANTED="-pg"
   AC_MSG_RESULT([yes])
else
   GPROF_WANTED=
   AC_MSG_RESULT([no])
fi

# APL: gcov coverage testing ?
AC_MSG_CHECKING([whether gcov coverage testing shall be enabled])
AC_ARG_VAR(GCOV_WANTED,
   [ enable gcov (-fprofile-arcs and -ftest-coverage) (see README-2-configure) default: no ])
if test "x$GCOV_WANTED" = "xyes"; then
   GCOV_WANTED="--coverage"
   AC_MSG_RESULT([yes])
else
   GCOV_WANTED=
   AC_MSG_RESULT([no])
fi

AC_DEFINE_UNQUOTED([GPROF_WANTED], [-pg],
                   [ define to enable gprof profiling ])


# APL: dynamic logging desired ?
AC_MSG_CHECKING([whether dynamic logging shall be enabled])
AC_ARG_VAR(DYNAMIC_LOG_WANTED,
   [ enable dynamic logging (see README-2-configure) default: no ])
if test "x$DYNAMIC_LOG_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([DYNAMIC_LOG_WANTED], [yes],
                   [ define to enable dynamic logging ])
else DYNAMIC_LOG_WANTED="no"
fi
AC_MSG_RESULT([$DYNAMIC_LOG_WANTED])


# APL: tracing of value flags (produces lots of messages) desired ?
AC_MSG_CHECKING([whether value flags shall be traced])
AC_ARG_VAR(VF_TRACING_WANTED,
   [ enable tracing of value flags (see README-2-configure) default: no ])
if test "x$VF_TRACING_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VF_TRACING_WANTED], [yes],
                   [ define to enable tracing of value flags ])
else VF_TRACING_WANTED="no"
fi
AC_MSG_RESULT([$VF_TRACING_WANTED])


# APL: desired assert level ?
AC_MSG_CHECKING([how picky internal tests shall be])
AC_ARG_VAR(ASSERT_LEVEL_WANTED,
   [ Assert level (see README-2-configure) default: 1) ])
if test "x$ASSERT_LEVEL_WANTED" = "x"; then ASSERT_LEVEL_WANTED=1; fi
AC_DEFINE_UNQUOTED([ASSERT_LEVEL_WANTED], [$ASSERT_LEVEL_WANTED],
                   [assert level])
AC_MSG_RESULT([$ASSERT_LEVEL_WANTED])

# APL: desired security level ?
AC_MSG_CHECKING([security level (0=none, 1=configured in preferences, 2=all)])
AC_ARG_VAR(SECURITY_LEVEL_WANTED,
   [ Assert level (see README-2-configure) default: 0) ])
if test "x$SECURITY_LEVEL_WANTED" = "x"; then SECURITY_LEVEL_WANTED=0; fi
AC_DEFINE_UNQUOTED([SECURITY_LEVEL_WANTED], [$SECURITY_LEVEL_WANTED],
                   [security level])
AC_MSG_RESULT([$SECURITY_LEVEL_WANTED])

# APL: number of cores ?
AC_MSG_CHECKING([how many CPU cores shall be used by APL (experimental!)])
AC_ARG_VAR(CORE_COUNT_WANTED,
   [ Core count (see README-2-configure) default: 0) ])
if test "x$CORE_COUNT_WANTED" = "x";     then CORE_COUNT_WANTED=0;  fi
if test "x$CORE_COUNT_WANTED" = "xall";  then CORE_COUNT_WANTED=-1; fi
if test "x$CORE_COUNT_WANTED" = "xALL";  then CORE_COUNT_WANTED=-1; fi
if test "x$CORE_COUNT_WANTED" = "xargv"; then CORE_COUNT_WANTED=-2; fi
if test "x$CORE_COUNT_WANTED" = "xARGV"; then CORE_COUNT_WANTED=-2; fi
if test "x$CORE_COUNT_WANTED" = "xsyl";  then CORE_COUNT_WANTED=-3; fi
if test "x$CORE_COUNT_WANTED" = "xSYL";  then CORE_COUNT_WANTED=-3; fi
AC_DEFINE_UNQUOTED([CORE_COUNT_WANTED], [$CORE_COUNT_WANTED],
                   [core count])
AC_MSG_RESULT([$CORE_COUNT_WANTED])

# APL: APserver transport ?
AC_MSG_CHECKING([which type of socket APserver shall use])
AC_ARG_VAR(APSERVER_TRANSPORT,
   [ APserver transport (see README-2-configure) default: TCP) ])
if test "x$APSERVER_TRANSPORT" = "x";        then APSERVER_TRANSPORT=0;  fi
if test "x$APSERVER_TRANSPORT" = "xTCP";     then APSERVER_TRANSPORT=0;  fi
if test "x$APSERVER_TRANSPORT" = "xLINUX";   then APSERVER_TRANSPORT=1;  fi
if test "x$APSERVER_TRANSPORT" = "xUNIX";    then APSERVER_TRANSPORT=2;  fi
AC_DEFINE_UNQUOTED([APSERVER_TRANSPORT], [$APSERVER_TRANSPORT],
                   [APserver transport: TCP=0, LINUX=1, UNIX=2])
AC_MSG_RESULT([$APSERVER_TRANSPORT])

# APL: APserver TCP port ?
AC_MSG_CHECKING([which port number APserver shall use for TCP or UDP sockets])
AC_ARG_VAR(APSERVER_PORT,
   [ APserver transport (see README-2-configure) default: 16366) ])
if test "x$APSERVER_PORT" = "x";   then APSERVER_PORT=16366;  fi
AC_DEFINE_UNQUOTED([APSERVER_PORT], [$APSERVER_PORT],
                   [APserver TCP listen port])
AC_MSG_RESULT([$APSERVER_PORT])

# APL: APserver port name ?
AC_MSG_CHECKING([which unix port name APserver shall use])
AC_ARG_VAR(APSERVER_PATH,
   [ APserver transport (see README-2-configure) default: /tmp/GNU-APL/APserver) ])
if test "x$APSERVER_PATH" = "x";   then APSERVER_PATH="/tmp/GNU-APL/APserver";  fi
AC_DEFINE_UNQUOTED([APSERVER_PATH], ["$APSERVER_PATH"],
                   [APserver listen port name])
AC_MSG_RESULT([$APSERVER_PATH])

# APL: desired cellcount of short values ?
AC_MSG_CHECKING([how long a short APL value shall be at most])
AC_ARG_VAR(SHORT_VALUE_LENGTH_WANTED,
    [ Cellcount of short values (see README-2-configure) default: 12) ])
if test "x$SHORT_VALUE_LENGTH_WANTED" = "x"; then SHORT_VALUE_LENGTH_WANTED=12; fi
AC_DEFINE_UNQUOTED([SHORT_VALUE_LENGTH_WANTED], [$SHORT_VALUE_LENGTH_WANTED],
                   [short value cellcount])
AC_MSG_RESULT([$SHORT_VALUE_LENGTH_WANTED])


# APL: visible markers (debug only option) ?
AC_MSG_CHECKING([whether visible markers shall be used])
AC_ARG_VAR(VISIBLE_MARKERS_WANTED,
   [ enable visible formatting markers (debug facility, see README-2-configure) default: no ])
if test "x$VISIBLE_MARKERS_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VISIBLE_MARKERS_WANTED], [yes],
                   [ define to have visible markers (DONT!)])
else VISIBLE_MARKERS_WANTED="no"
fi
AC_MSG_RESULT([$VISIBLE_MARKERS_WANTED])

# APL: support for rational numbers ?
AC_MSG_CHECKING([whether support for rational numbers is desired ])
AC_ARG_VAR(RATIONAL_NUMBERS_WANTED,
   [ enable support for rational numbers (see README-2-configure) default: no ])
if test "x$RATIONAL_NUMBERS_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([RATIONAL_NUMBERS_WANTED], [yes],
                   [ define to have support for rational numbers (EXPERIMENTAL!)])
else RATIONAL_NUMBERS_WANTED="no"
fi
AC_MSG_RESULT([$RATIONAL_NUMBERS_WANTED])

# check if we are on i686 (to fix a strange rpmbuild problem)
AC_MSG_CHECKING([whether we compile for an i686 CPU])
AM_CONDITIONAL([ON_i686], [test $( uname -m) = i686])
if test `uname -m` = "i686" ; then
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

# check if we want development options
AM_CONDITIONAL([DEVELOP],  [test x$DEVELOP_WANTED    = xyes])
AM_CONDITIONAL([SQLITE3],  [test x$found_sqlite      = xyes])
AM_CONDITIONAL([POSTGRES], [test x$USABLE_PostgreSQL = xyes])

echo
AC_OUTPUT

echo

# create buildtag.hh and set apl_top to the directory that contains buildtag.sh
#
echo "configure: creating buildtag.hh..."
apl_top=""
for f in "." ".." "../.." "../../.."  "../../../.."
do
   if test -f "$f/buildtag.sh"
   then
      apl_top="$f"
      . "$f/buildtag.sh"
      abs_apl_top=$(readlink -f "$apl_top")
   fi
done

if test -z "$apl_top"
then
   echo "configure: *** could not find top-level GNU APL directory, using '.'"
   apl_top="."
else
   echo "configure: top-level apl directory is: $abs_apl_top"
fi

# remove makefile.h
#
if test -f "$apl_top/makefile.h"
then
   echo "configure: removing old $apl_top/makefile.h"
   rm -f "$apl_top/makefile.h"
fi

# summary
  echo " "
if test "x$USABLE_PostgreSQL" != "xyes" -a "x$want1_postgresql" = "xyes"; then
  echo "NOTE: PostgreSQL support was requested, but the native function for "
  echo "      accessing PostgreSQL cannot be built because your PostgreSQL "
  echo "      installation appears to be missing or incomplete (see details above)"
  echo " "
fi

 if test "x$found_sqlite" != "xyes" -a "x$WANT1_SQLITE3" = "xyes"; then
  echo "NOTE: SQLite support was requested, but the native function for "
  echo "      accessing SQLite cannot not be built because your SQLite "
  echo "      installation appears to be missing or incomplete (see details above)"
  echo " "
fi

