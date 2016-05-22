#!/bin/sh

VERSION=1
REV=53
DATE=$(date)
SYSTEM=$(uname -a)
MACHINE=$(uname -m)
OS=$(uname)

if [[ -d ".git" ]] || [[ -d "../../.git" ]]; then
    REV="${REV}~"
    if git status | grep -q "modified:" &>/dev/null ; then
        REV="${REV}M"
    fi
    REV="${REV}$(git rev-list HEAD -n 1 | head -c 7)"
fi

echo "#define VERSION \"${VERSION}.${REV}\"" > version.h
echo "#define NUMBER_VERSION \"${VERSION}.${REV}\"" >> version.h
echo "#define SYSTEM_VERSION \"$SYSTEM\"" >> version.h
echo "#define BUILD_DATE \"$DATE\"" >> version.h
echo "#define MACHINE \"$MACHINE\"" >> version.h
echo "#define OS \"$OS\"" >> version.h

