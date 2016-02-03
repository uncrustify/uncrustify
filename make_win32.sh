#! /bin/sh
#
# Builds the 32-bit Windows release zip using mingw.
# Tested only on Ubuntu 14.04 x86_64 host.
# You'll need to install the "mingw-w64" package.
#

make distclean

# This is the prefix for the mingw32 build
HOST_PREFIX=i686-w64-mingw32

SRC_DIR=$PWD
cd ..
BLD_DIR=$PWD/build-$HOST_PREFIX
REL_DIR=$PWD/release

echo "Configuring for Win32 build..."
rm -rf $BLD_DIR
[ -d $BLD_DIR ] || mkdir $BLD_DIR
cd $BLD_DIR
$SRC_DIR/configure --srcdir=$SRC_DIR --host=$HOST_PREFIX CXXFLAGS="-I$SRC_DIR/win32 -static"

# build the version string from git
cd $SRC_DIR
python make_version.py > /dev/null

# extract the version from src/uncrustify_version.h...
VERSION=`grep '#define UNCRUSTIFY_VERSION ' src/uncrustify_version.h | \
         sed -e "s/#define UNCRUSTIFY_VERSION//" -e 's/\"//g' -e 's/ //g'`
[ -d $REL_DIR ] || mkdir -p $REL_DIR
cd $SRC_DIR

VERDIR=$REL_DIR/uncrustify-$VERSION-win32
THEZIP=$REL_DIR/$(basename $VERDIR).zip

echo
echo "Building version $VERSION for Win32"

cd $BLD_DIR
make clean
make

cd $SRC_DIR
if [ -e $VERDIR ] ; then
  rm -rf $VERDIR
fi
mkdir $VERDIR $VERDIR/cfg $VERDIR/doc

cp $BLD_DIR/src/uncrustify.exe     $VERDIR/
cp README* BUGS ChangeLog          $VERDIR/
cp etc/*.cfg                       $VERDIR/cfg/
cp documentation/htdocs/index.html $VERDIR/doc/

$HOST_PREFIX-strip $VERDIR/uncrustify.exe

[ -e $THEZIP ] && rm -f $THEZIP

cd $REL_DIR
zip -r9 $THEZIP $(basename $VERDIR)/*
cd $THISDIR

echo "Stored in $THEZIP"
