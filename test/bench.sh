#!/bin/bash

# bench.sh
# Benchmark the rt stuff ...

RESDIR=./results
BINDIR=../src
FS_ROOT=/vagrant/hpc-clone/rt-data
FS_CFG=../config/fs_6.4K_verysmall.cfg
FS_NOBJS_READ=10000

#nthreads="1 2 4 8 16 32 64 128"
nthreads="1 2 4"

if [[ $# -ne 2 ]]
then
  echo "Usage: bench.sh <scenario> <fs-dir>"
  echo "       <scenario> is the results scenario"
  echo "       <fs-dir> is the filesystem directory / mount point to use (must not exist)"
  exit
fi

scen=$1
fsdir=$2

if [[ -d ${RESDIR}/$scen ]]
then
  echo "Results scenario $scen already exists.  Exiting"
  exit
fi

mkdir -p ${RESDIR}/$scen

# write

mkdir -p ${FS_ROOT}/rt

for t in ${nthreads}
do
  rm -rf ${FS_ROOT}/rt/*
  ${BINDIR}/gentree -t $t -c ${FS_CFG} -f ${FS_ROOT}/rt \
      | tee ${RESDIR}/${scen}/gentree.hdd.$t
done

# read

for t in ${nthreads}
do
  THREAD_NOBJS_READ=$((${FS_NOBJS_READ} / ${t}))
  sudo sync
  sudo /sbin/sysctl vm.drop_caches=3
  ${BINDIR}/readtree -t $t -n ${THREAD_NOBJS_READ} -c ${FS_CFG} ${FS_ROOT}/rt \
      | tee ${RESDIR}/${scen}/readtree.hdd.$t
done


