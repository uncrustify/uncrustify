#! /bin/sh
#
# Builds the release zip for Windows using mingw
# On ubuntu, you'll need to install the mingw32 package.
#

# grab the version - there is probably an easier way...
VERSION=`grep '#define UNCRUSTIFY_VERSION ' src/uncrustify_version.h | \
         sed -e "s/#define UNCRUSTIFY_VERSION//" -e 's/\"//g' -e 's/ //g'`
VERDIR=uncrustify-$VERSION-win32
RELDIR=../release
THEZIP=$RELDIR/$VERDIR.zip

echo "Building version $VERSION for Win32"

./configure --host=i586-mingw32msvc

make clean
make

if [ -e $VERDIR ] ; then
  rm -rf $VERDIR
fi
mkdir $VERDIR

cp src/uncrustify.exe              $VERDIR/
cp etc/*.cfg                       $VERDIR/
cp ChangeLog                       $VERDIR/
cp documentation/htdocs/index.html $VERDIR/

strip $VERDIR/uncrustify.exe

if ! [ -e $RELDIR ] ; then
  mkdir $RELDIR
fi
[ -e $THEZIP ] && rm -f $THEZIP

zip -r9 $THEZIP $VERDIR

echo "Stored in $THEZIP"
