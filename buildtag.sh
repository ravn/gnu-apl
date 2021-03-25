
#
# this script writes a build tag into file buildtag.hh
# it is being source'd by ./configure
#

# locate top-level apl directiory.
# $abs_top_srcdir and friends do not work here.

apl_top="$PWD"
while true
do
   if [ "$apl_top" = "/" ]  # bad luck
   then
      echo "*** No directory apl-* in $PWD. keeping old buildtag."
      return 0
   fi

   dir=$(basename $apl_top)
   if [ "${dir:0:4}" = "apl-" ]
   then
      break
   fi
   apl_top=$(dirname "$apl_top")
done

if [ -z `which svnversion` ]
then
   echo "*** svnversion not installed: keeping old buildtag"
   return 0
fi

SVNINFO=$(svnversion)
if [ "$SVNINFO" = "Unversioned directory" ]
then
   echo "*** current directory is not an SVN checkout: keeping old buildtag"
   return 0
fi

if [ -z $(which svn) ]
then
   echo "*** subversion not installed: keeping old buildtag"
   return 0
fi

PACKAGE_NAME=$1
PACKAGE_VERSION=$2

ARCHIVE_SVNINFO=`svn info "$apl_top"/src/Archive.cc | grep "Last Changed Rev" \
                                                    | awk -F : '{print $2;}'`

CONFIGURE_OPTS="unknown ./configure options (no config.status)"
if [ -x ./config.status ]
then
    CONFIGURE_OPTS=`./config.status -V  | \
                    grep "with options" | \
                    awk -F\" '{print $2;}'`
    if [ "$CONFIGURE_OPTS" == "" ]
    then
        CONFIGURE_OPTS="default ./configure options"
    fi
fi

if [ -n "$SOURCE_DATE_EPOCH" ]
then
    BUILD_DATE=`date -u "+%F %R:%S %Z" --date=@$SOURCE_DATE_EPOCH`
    BUILD_OS=""
else
    BUILD_DATE=`date -u "+%F %R:%S %Z"`
    BUILD_OS=`uname -s -r -m`
fi

BUILD_TAG="\" / $SVNINFO\", \"$BUILD_DATE\", \"$BUILD_OS\", \"$CONFIGURE_OPTS\""

# write buildtag.hh
#
echo "#include \"Common.hh\""                                     > buildtag.hh
echo "#define BUILDTAG PACKAGE_NAME, PACKAGE_VERSION $BUILD_TAG" >> buildtag.hh
echo "#define ARCHIVE_SVN \"$ARCHIVE_SVNINFO\""                  >> buildtag.hh

# write makefile.h
#
echo "// some strings exported from Makefile"             > makefile.h
echo "//"                                                >> makefile.h
echo "#define Makefile__bindir     \"$bindir\""          >> makefile.h
echo "#define Makefile__docdir     \"$docdir\""          >> makefile.h
echo "#define Makefile__sysconfdir \"$sysconfdir\""      >> makefile.h
echo "#define Makefile__pkglibdir  \"$libdir/$PACKAGE\n" >> makefile.h
echo "#define Makefile__localedir  \"$localedir\""       >> makefile.h
echo "#define Makefile__srcdir     \"$abs_srcdir\""      >> makefile.h
echo                                                     >> makefile.h

