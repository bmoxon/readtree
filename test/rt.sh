#!/bin/bash

if [[ $# -ne 3 ]]
then
  echo "Usage: rt <cfgfile> <path> <nfiles-to-read>"
  exit
fi

CFGFILE=$1
echo "Using config file: ${CFGFILE}"
ROOTPATH=$2
echo "Using root path: ${ROOTPATH}"
NFILES_TO_READ=$3

BINDIR=../src

SRCDIRTREE=${ROOTPATH}

if [[ -e ${SRCDIRTREE} ]]
then
  echo "$SRCDIRTREE exists. Exiting"
  exit
fi

mkdir -p ${SRCDIRTREE}

# sync before we start

sudo sync

$BINDIR/gentree ${SRCDIRTREE} -c ${CFGFILE}

# sync and clear the page cache after writing the tree
#sudo sh -c "sync && echo 1 > /proc/sys/vm/drop_caches"

echo "syncing and dropping caches ..."
echo "sudo sync"
sudo sync
echo "sudo /sbin/sysctl vm.drop_caches=3"
sudo /sbin/sysctl vm.drop_caches=3

$BINDIR/readtree -n ${NFILES_TO_READ} -c ${CFGFILE} ${SRCDIRTREE}

