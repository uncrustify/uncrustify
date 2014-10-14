#! /bin/sh
#
# Builds the 32-bit Windows release zip using mingw.
# Tested only on Ubuntu 14.04 x86_64 host.
# You'll need to install the "mingw-w64" package.
#

# This is the prefix for the mingw32 build
HOST_PREFIX=i686-w64-mingw32

echo "Configuring for Win32 build..."
./configure --host=$HOST_PREFIX CXXFLAGS="-I../win32 -static"

# build the version string from git
python make_version.py > /dev/null

# extract the version from src/uncrustify_version.h...
VERSION=`grep '#define UNCRUSTIFY_VERSION ' src/uncrustify_version.h | \
         sed -e "s/#define UNCRUSTIFY_VERSION//" -e 's/\"//g' -e 's/ //g'`
THISDIR=$PWD
RELDIR=../release
if ! [ -e $RELDIR ] ; then
  mkdir -p $RELDIR
fi
# convert RELDIR to an absolute path
cd $RELDIR
RELDIR=$PWD
cd $THISDIR

VERDIR=$RELDIR/uncrustify-$VERSION-win32
THEZIP=$RELDIR/$(basename $VERDIR).zip

echo
echo "Building version $VERSION for Win32"

make clean
make

if [ -e $VERDIR ] ; then
  rm -rf $VERDIR
fi
mkdir $VERDIR $VERDIR/cfg $VERDIR/doc

cp src/uncrustify.exe              $VERDIR/
cp README BUGS ChangeLog           $VERDIR/
cp etc/*.cfg                       $VERDIR/cfg/
cp documentation/htdocs/index.html $VERDIR/doc/

$HOST_PREFIX-strip $VERDIR/uncrustify.exe

[ -e $THEZIP ] && rm -f $THEZIP

cd $RELDIR
zip -r9 $THEZIP $(basename $VERDIR)/*
cd $THISDIR

echo "Stored in $THEZIP"
