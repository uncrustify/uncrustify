#!/bin/sh
#
# ARGS: 1:remote
#

BRANCH=coverity_scan

if [ -z "$1" ] ; then
	cat <<EOF
Usage: $0 REMOTE

This script triggers a coverity build by pushing the current code to
the '$BRANCH' branch.

It copies the current (master) branch over the '$BRANCH' branch,
then copies 'coverity.travis.yml' over '.travis.yml' and force-pushes
the new branch.

You obviously must have commit rights on the repository, so this is
a maintainer-only script, unless you are pushing to your own fork.

Example:
  $0 origin
EOF
	exit 1
fi
REMOTE=$1

set -e

if [ -z "$NOTIFICATION_EMAIL" ] ; then
	NOTIFICATION_EMAIL=$(git config user.email)
	if [ -z "$NOTIFICATION_EMAIL" ] ; then
		echo "No notification email address set."
		exit 1
	fi
fi

if [ $(git rev-parse --abbrev-ref HEAD) != 'master' ] ; then
	cat <<EOF
Please switch to the master branch before running this script.
EOF
	exit 1
fi

if git describe --dirty | grep -q dirty ; then
	cat <<EOF
Please clean up your dirty workspace before running this script.
EOF
	exit 1
fi

echo "NOTIFICATION_EMAIL: $NOTIFICATION_EMAIL"

if git branch | grep $BRANCH ; then
	echo "Deleting local coverity_scan branch..."
	git branch -D $BRANCH
fi

git branch $BRANCH $REMOTE/master
git checkout -f $BRANCH

sed "s|{NOTIFICATION_EMAIL}|$NOTIFICATION_EMAIL|" coverity.travis.yml > .travis.yml

git add .travis.yml
git commit -m 'Copy coverity.travis.yml -> .travis.yml for coverity build.'
git push -f $REMOTE $BRANCH
git checkout master

echo 'Finished.'
