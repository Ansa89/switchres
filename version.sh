#!/bin/sh

VERSION="1"
VER="52"
DATE=`date`
SYSTEM=`uname -a`
MACHINE=`uname -m`
OS=`uname`
LOCALVER=0

if [[ -d ".git" ]] || [[ -d "../../.git" ]]; then
    git rev-list HEAD | sort > config.git-hash
    LOCALVER=`wc -l config.git-hash | awk '{print $1}'`
fi
if [[ $LOCALVER > 1 ]] ; then
    VER=`git rev-list origin/master | sort | join config.git-hash - | wc -l | awk '{print $1}'`
    if [ $VER != $LOCALVER ] ; then
        VER="$VER+$(($LOCALVER-$VER))"
    elif git status | grep -q "modified:" ; then
        VER="${VER}M"
    fi
    VER="$VER-$(git rev-list HEAD -n 1 | head -c 7)"
    echo "#define VERSION \"${VERSION}.${VER}\"" > version.h
    rm -f config.git-hash
else
    echo "#define VERSION \"${VERSION}.${VER}\"" > version.h
    VER="x"
fi
echo "#define NUMBER_VERSION \"$VERSION.$VER\"" >>version.h
echo "#define SYSTEM_VERSION \"$SYSTEM\"" >>version.h
echo "#define BUILD_DATE \"$DATE\"" >>version.h
echo "#define MACHINE \"$MACHINE\"" >>version.h
echo "#define OS \"$OS\"" >>version.h

