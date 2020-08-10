# readtree
Simple filetree create/read tool

## Overview

readtree is a simple tool for generating file data in a large file tree
and reading it. It includes two basic tools:

```
. gentree: generate a file tree at the specified path
. readtree: randomly read a set of files from the specified path
```

### config file

A filetree configuration file is first defined that describes the file tree
to be used. The configuration file includes the following entries
in parameter value pairs (separated by whitespace, one pair per line):

```
NLEVELS		Number of directory levels
FANOUT		Number of child directories at each level
NFILESNODE	Number of files in non-leaf nodes in the tree
NFILESLEAF	Number of files at leaf nodes in the tree
FILESIZEMU	Average file size
FILESIZESD	File size standard deviation
```

### gentree

Create a tree with specified fan-out, # files per dir, and file size.

```
$ ./gentree
Usage (Files): gentree [-t <nthreads>] -c <cfgfilename> [-f] <rootpath>
       -t <nthreads>    is the number of write threads(8)
       -c <cfgfilename> is gentree config file
       -f use fsync() for filesystem writes
       <rootpath> is the tree root
```

This creates a file tree rooted at <rootpath> with a directory/file
structure as described by the config file.  A simple example, fs_10.cfg:

```
# fs_10.cfg
NLEVELS	1
FANOUT	2
NFILESNODE	0
NFILESLEAF	5
FILESIZEMU	300
FILESIZESD	100
```

will create 1 toplevel directory with 2 subdirectories (fanout = 2), each
of which has 5 files in it.  Files will have a randomized size around
the specified mean (FILESIZEMU) within 3x the specified standard deviation
(FILESIZESD).  E.g.

```
$ ./gentree -t 2 -c ../config/fs_10.cfg /vagrant/hpc-clone/rt-data
Got nlevels = 1, fanout = 2
    nfilesnode = 0, nfilesleaf = 5
    filesizemu = 300, filesizesd = 100
[0]11

Gentree: 2 threads

Gentree: nFiles = 10, nBytes =2753
         cumulative procTime =    0.004
         elapsed time():    0.003 secs
         write rate = 3466 objs/s

         min latency = 295 us
         max latency = 519 us
         avg latency = 366 us
         2ms+ latency count = 0

         latency hist[0..1999] = 10
         latency hist[2000..3999] = 0
         latency hist[4000..5999] = 0
         latency hist[6000..7999] = 0
         latency hist[8000..9999] = 0
         latency hist[10000..11999] = 0
         latency hist[12000..13999] = 0
         latency hist[14000..15999] = 0
         latency hist[16000..17999] = 0
         latency hist[18000..19999] = 0
         latency hist[20000..21999] = 0
         latency hist[22000..23999] = 0
         latency hist[24000..25999] = 0
         latency hist[26000..27999] = 0
         latency hist[28000..29999] = 0
         latency hist[30000..31999] = 0
         latency hist[32000..33999] = 0
         latency hist[34000..35999] = 0
         latency hist[36000..37999] = 0
         latency hist[38000..39999] = 0
Done ... exiting
```

The generated data for this scenario looks like:

```
$ tree -ps /vagrant/hpc-clone/rt-data
/vagrant/hpc-clone/rt-data
├── [drwxr-xr-x         224]  00
│   ├── [-rw-------         328]  00000000.dat
│   ├── [-rw-------         186]  00000001.dat
│   ├── [-rw-------         154]  00000002.dat
│   ├── [-rw-------         420]  00000003.dat
│   └── [-rw-------         417]  00000004.dat
└── [drwxr-xr-x         224]  01
    ├── [-rw-------         185]  00000000.dat
    ├── [-rw-------         288]  00000001.dat
    ├── [-rw-------         201]  00000002.dat
    ├── [-rw-------         333]  00000003.dat
    └── [-rw-------         241]  00000004.dat
```

A more aggressive test using the following config file:

```
NLEVELS	3
FANOUT	4
NFILESNODE	20
NFILESLEAF	100
FILESIZEMU	3K
FILESIZESD	1K
```

will generate a tree with:

```
. 3 directory levels, e.g. 00/00/00
. fanout of 4 - 4 directories in each parent directory at each level, e.g.
    00/00/00, 00/00/01, 00/00/02, 00/00/03, 00/01/00, 00/01/01, ...
. 20 files in each non-leaf directory
. 100 files in each of the leaf directories (i.e. 3 levels down)
. average file size 3K bytes, std deviation of file size 1K
```

### readtree

readtree reads files from the tree described by the (same) config file.
It uses a number of read threads, EACH of which will randomly read the
specified number of files from the tree.  Where nfiles << tree size, this
is a "sparse" read.  Where nfiles >> tree size, this is a "dense" read
that may (randomly) re-read files (potentially from a client-side disk
buffer cache and/or target-side file cache). readtree also supports
parameterization to include filesystem metadata ops (stat in this case)
and sleep times between successive reads in each thread, if desired.

```
$ ./readtree
Usage (Files): readtree [-t <nthreads>] -n <nfiles> -m <metapct> -c <cfgfilename> <rootpath>
       -t <nthreads>    is the number of read threads
       -n <nfiles>      is the number of files to read PER THREAD
       -m <metapct>     is the percentage of metadata ops to perform
       -s <sleepus>     is the number of microseconds to sleep between IOPS (0)
       -c <cfgfilename> is gentree config file
       <rootpath> is the tree root
```

For the small, 10-file tree we looked at earlier:

```
$ ./readtree -t 8 -n 20 -m 10 -c ../config/fs_10.cfg /vagrant/hpc-clone/rt-data
Got nlevels = 1, fanout = 2
    nfilesnode = 0, nfilesleaf = 5
    filesizemu = 300, filesizesd = 100
readtree: reading contents of 20 randomly selected files
          in each of 8 threads
...
pthread_join complete

Readtree: nMeta = 15, nFiles = 145, nBytes = 36557
          cumulative procTime =    0.054
          elapsed time():    0.019 secs
          read rate = 7809 objs/s

          min latency = 105 us
          max latency = 1267 us
          avg latency = 369 us
          2ms+ latency count = 0

          latency hist[0..1999] = 145
          latency hist[2000..3999] = 0
          latency hist[4000..5999] = 0
          latency hist[6000..7999] = 0
          latency hist[8000..9999] = 0
          latency hist[10000..11999] = 0
          latency hist[12000..13999] = 0
          latency hist[14000..15999] = 0
          latency hist[16000..17999] = 0
          latency hist[18000..19999] = 0
          latency hist[20000..21999] = 0
          latency hist[22000..23999] = 0
          latency hist[24000..25999] = 0
          latency hist[26000..27999] = 0
          latency hist[28000..29999] = 0
          latency hist[30000..31999] = 0
          latency hist[32000..33999] = 0
          latency hist[34000..35999] = 0
          latency hist[36000..37999] = 0
          latency hist[38000..39999] = 0

Data check ...
[ 0]: /vagrant/hpc-clone/rt-data/00/00000004.dat (417)
[ 1]: /vagrant/hpc-clone/rt-data/00/00000004.dat (417)
[ 2]: /vagrant/hpc-clone/rt-data/00/00000003.dat (420)
[ 3]: /vagrant/hpc-clone/rt-data/01/00000004.dat (241)
[ 4]: /vagrant/hpc-clone/rt-data/01/00000003.dat (333)
[ 5]: /vagrant/hpc-clone/rt-data/00/00000002.dat (154)
[ 6]: /vagrant/hpc-clone/rt-data/01/00000002.dat (201)
[ 7]: /vagrant/hpc-clone/rt-data/01/00000002.dat (201)
[ 8]: /vagrant/hpc-clone/rt-data/01/00000002.dat (201)
[ 9]: /vagrant/hpc-clone/rt-data/01/00000000.dat (185)
[10]: /vagrant/hpc-clone/rt-data/00/00000001.dat (186)
[11]: /vagrant/hpc-clone/rt-data/00/00000001.dat (186)
[12]: /vagrant/hpc-clone/rt-data/00/00000001.dat (186)
[13]: /vagrant/hpc-clone/rt-data/00/00000000.dat (328)
[14]: /vagrant/hpc-clone/rt-data/01/00000001.dat (288)
Freeing thread structs
Freeing global arrays
Done ... exiting
```

### run stats

Both gentree and readtree generate statistics describing the run.  E.g. in
the above run, readtree made 160 reads, 15 of which were metadata only.
The total bytes read (across all threads) was 36557 bytes.  The cumulative
processing time is the sum of the per-thread processing times.  The
elapsed time is the end-to-end (wallclock) time.  With multiple threads
and an appropriately performant "backend" (which could be local disk,
NFS server, or other high performance file system), the cumulative
processing time should exceed the elapsed time (i.e. concurrency > 1).
A total read rate (across all threads) is reported, as well as latency
statistics and a latency distribution histogram.

## Scaling the tests up and out

gentree and readtree can be used in conjunction with tools such as gnu
parallel -- for multiple process instances -- and clush to generate
arbitrarily scaled workloads against one or more target volumes or servers.

For example, N copies of gentree can be aimed at a single backend volume
with each gentree generating its own tree:

```
parallel -j 10 gentree <args> /mnt/vol/d{} ::: {1..10}
```

N copies can be aimed at multiple backend volumes (e.g. in a parallel file
system exposing multiple volumes in a distributed fashion across backend
servers):

```
parallel -j 10 gentree <args> /mnt/vol{} ::: {1..10}
```

readtree operations could be similarly distributed, or could be concentrated
against a single backend volume

```
parallel -j 10 readtree <args> /mnt/vol1
```

## Wrapper scripts

Wrapper scripts can be created to run the scaled up/out scenarios.  An
example is at test/bench.sh that "ramps" across 1, 2, and 4 threads for
both tree generation (removing the created trees in between) and reads
(in this case, purging the disk buffer cache between iterations).


```
$ ./bench.sh vs-test /vagrant/hpc-clone/rt-data/vs
Got nlevels = 3, fanout = 4
    nfilesnode = 0, nfilesleaf = 100
    filesizemu = 300, filesizesd = 100
[0][1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333

Gentree: 1 threads

Gentree: nFiles = 6400, nBytes =1912113
         cumulative procTime =    2.025
         elapsed time():    2.131 secs
         write rate = 3002 objs/s

         min latency = 166 us
         max latency = 5748 us
         avg latency = 316 us
         2ms+ latency count = 5

         latency hist[0..1999] = 6395
         latency hist[2000..3999] = 3
         latency hist[4000..5999] = 2
         latency hist[6000..7999] = 0
         latency hist[8000..9999] = 0
         latency hist[10000..11999] = 0
         latency hist[12000..13999] = 0
         latency hist[14000..15999] = 0
         latency hist[16000..17999] = 0
         latency hist[18000..19999] = 0
         latency hist[20000..21999] = 0
         latency hist[22000..23999] = 0
         latency hist[24000..25999] = 0
         latency hist[26000..27999] = 0
         latency hist[28000..29999] = 0
         latency hist[30000..31999] = 0
         latency hist[32000..33999] = 0
         latency hist[34000..35999] = 0
         latency hist[36000..37999] = 0
         latency hist[38000..39999] = 0
Done ... exiting
Got nlevels = 3, fanout = 4
    nfilesnode = 0, nfilesleaf = 100
    filesizemu = 300, filesizesd = 100
[0][1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333

Gentree: 2 threads

Gentree: nFiles = 6400, nBytes =1910495
         cumulative procTime =    2.914
         elapsed time():    1.490 secs
         write rate = 4296 objs/s

         min latency = 174 us
         max latency = 5155 us
         avg latency = 455 us
         2ms+ latency count = 39

         latency hist[0..1999] = 6361
         latency hist[2000..3999] = 35
         latency hist[4000..5999] = 4
         latency hist[6000..7999] = 0
         latency hist[8000..9999] = 0
         latency hist[10000..11999] = 0
         latency hist[12000..13999] = 0
         latency hist[14000..15999] = 0
         latency hist[16000..17999] = 0
         latency hist[18000..19999] = 0
         latency hist[20000..21999] = 0
         latency hist[22000..23999] = 0
         latency hist[24000..25999] = 0
         latency hist[26000..27999] = 0
         latency hist[28000..29999] = 0
         latency hist[30000..31999] = 0
         latency hist[32000..33999] = 0
         latency hist[34000..35999] = 0
         latency hist[36000..37999] = 0
         latency hist[38000..39999] = 0
Done ... exiting
Got nlevels = 3, fanout = 4
    nfilesnode = 0, nfilesleaf = 100
    filesizemu = 300, filesizesd = 100
[0][1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333[1][2]3333[2]3333[2]3333[2]3333

Gentree: 4 threads

Gentree: nFiles = 6400, nBytes =1920722
         cumulative procTime =    4.311
         elapsed time():    1.092 secs
         write rate = 5860 objs/s

         min latency = 193 us
         max latency = 6537 us
         avg latency = 673 us
         2ms+ latency count = 24

         latency hist[0..1999] = 6376
         latency hist[2000..3999] = 20
         latency hist[4000..5999] = 3
         latency hist[6000..7999] = 1
         latency hist[8000..9999] = 0
         latency hist[10000..11999] = 0
         latency hist[12000..13999] = 0
         latency hist[14000..15999] = 0
         latency hist[16000..17999] = 0
         latency hist[18000..19999] = 0
         latency hist[20000..21999] = 0
         latency hist[22000..23999] = 0
         latency hist[24000..25999] = 0
         latency hist[26000..27999] = 0
         latency hist[28000..29999] = 0
         latency hist[30000..31999] = 0
         latency hist[32000..33999] = 0
         latency hist[34000..35999] = 0
         latency hist[36000..37999] = 0
         latency hist[38000..39999] = 0
Done ... exiting
vm.drop_caches = 3
Got nlevels = 3, fanout = 4
    nfilesnode = 0, nfilesleaf = 100
    filesizemu = 300, filesizesd = 100
readtree: reading contents of 10000 randomly selected files
          in each of 1 threads
...
pthread_join complete

Readtree: nMeta = 0, nFiles = 10000, nBytes = 3013219
          cumulative procTime =    2.427
          elapsed time():    3.259 secs
          read rate = 3068 objs/s

          min latency = 84 us
          max latency = 2488 us
          avg latency = 242 us
          2ms+ latency count = 2

          latency hist[0..1999] = 9998
          latency hist[2000..3999] = 2
          latency hist[4000..5999] = 0
          latency hist[6000..7999] = 0
          latency hist[8000..9999] = 0
          latency hist[10000..11999] = 0
          latency hist[12000..13999] = 0
          latency hist[14000..15999] = 0
          latency hist[16000..17999] = 0
          latency hist[18000..19999] = 0
          latency hist[20000..21999] = 0
          latency hist[22000..23999] = 0
          latency hist[24000..25999] = 0
          latency hist[26000..27999] = 0
          latency hist[28000..29999] = 0
          latency hist[30000..31999] = 0
          latency hist[32000..33999] = 0
          latency hist[34000..35999] = 0
          latency hist[36000..37999] = 0
          latency hist[38000..39999] = 0

Data check ...
[ 0]: /vagrant/hpc-clone/rt-data/rt/02/01/03/00000006.dat (385)
[ 1]: /vagrant/hpc-clone/rt-data/rt/03/02/03/00000030.dat (342)
[ 2]: /vagrant/hpc-clone/rt-data/rt/01/03/03/00000027.dat (399)
[ 3]: /vagrant/hpc-clone/rt-data/rt/02/02/02/00000058.dat (299)
[ 4]: /vagrant/hpc-clone/rt-data/rt/03/03/00/00000085.dat (426)
[ 5]: /vagrant/hpc-clone/rt-data/rt/00/03/00/00000000.dat (430)
[ 6]: /vagrant/hpc-clone/rt-data/rt/03/03/03/00000053.dat (329)
[ 7]: /vagrant/hpc-clone/rt-data/rt/03/00/02/00000005.dat (304)
[ 8]: /vagrant/hpc-clone/rt-data/rt/03/02/02/00000090.dat (150)
[ 9]: /vagrant/hpc-clone/rt-data/rt/03/00/00/00000076.dat (439)
[10]: /vagrant/hpc-clone/rt-data/rt/02/03/00/00000006.dat (158)
[11]: /vagrant/hpc-clone/rt-data/rt/00/03/03/00000067.dat (244)
[12]: /vagrant/hpc-clone/rt-data/rt/03/00/00/00000050.dat (313)
[13]: /vagrant/hpc-clone/rt-data/rt/01/00/02/00000084.dat (240)
[14]: /vagrant/hpc-clone/rt-data/rt/01/00/02/00000035.dat (405)
Freeing thread structs
Freeing global arrays
Done ... exiting
vm.drop_caches = 3
Got nlevels = 3, fanout = 4
    nfilesnode = 0, nfilesleaf = 100
    filesizemu = 300, filesizesd = 100
readtree: reading contents of 5000 randomly selected files
          in each of 2 threads
...
pthread_join complete

Readtree: nMeta = 0, nFiles = 10000, nBytes = 2987527
          cumulative procTime =    2.717
          elapsed time():    1.806 secs
          read rate = 5538 objs/s

          min latency = 69 us
          max latency = 2170 us
          avg latency = 271 us
          2ms+ latency count = 4

          latency hist[0..1999] = 9996
          latency hist[2000..3999] = 4
          latency hist[4000..5999] = 0
          latency hist[6000..7999] = 0
          latency hist[8000..9999] = 0
          latency hist[10000..11999] = 0
          latency hist[12000..13999] = 0
          latency hist[14000..15999] = 0
          latency hist[16000..17999] = 0
          latency hist[18000..19999] = 0
          latency hist[20000..21999] = 0
          latency hist[22000..23999] = 0
          latency hist[24000..25999] = 0
          latency hist[26000..27999] = 0
          latency hist[28000..29999] = 0
          latency hist[30000..31999] = 0
          latency hist[32000..33999] = 0
          latency hist[34000..35999] = 0
          latency hist[36000..37999] = 0
          latency hist[38000..39999] = 0

Data check ...
[ 0]: /vagrant/hpc-clone/rt-data/rt/00/01/02/00000040.dat (439)
[ 1]: /vagrant/hpc-clone/rt-data/rt/00/03/00/00000050.dat (407)
[ 2]: /vagrant/hpc-clone/rt-data/rt/03/02/02/00000076.dat (310)
[ 3]: /vagrant/hpc-clone/rt-data/rt/02/02/02/00000081.dat (235)
[ 4]: /vagrant/hpc-clone/rt-data/rt/00/00/01/00000087.dat (270)
[ 5]: /vagrant/hpc-clone/rt-data/rt/00/00/00/00000060.dat (276)
[ 6]: /vagrant/hpc-clone/rt-data/rt/00/00/00/00000053.dat (347)
[ 7]: /vagrant/hpc-clone/rt-data/rt/00/02/02/00000062.dat (413)
[ 8]: /vagrant/hpc-clone/rt-data/rt/03/01/02/00000073.dat (321)
[ 9]: /vagrant/hpc-clone/rt-data/rt/02/00/03/00000088.dat (383)
[10]: /vagrant/hpc-clone/rt-data/rt/00/00/01/00000096.dat (354)
[11]: /vagrant/hpc-clone/rt-data/rt/03/01/02/00000021.dat (199)
[12]: /vagrant/hpc-clone/rt-data/rt/03/02/02/00000061.dat (167)
[13]: /vagrant/hpc-clone/rt-data/rt/02/02/03/00000007.dat (224)
[14]: /vagrant/hpc-clone/rt-data/rt/00/01/00/00000030.dat (205)
Freeing thread structs
Freeing global arrays
Done ... exiting
vm.drop_caches = 3
Got nlevels = 3, fanout = 4
    nfilesnode = 0, nfilesleaf = 100
    filesizemu = 300, filesizesd = 100
readtree: reading contents of 2500 randomly selected files
          in each of 4 threads
...
pthread_join complete

Readtree: nMeta = 0, nFiles = 10000, nBytes = 3022270
          cumulative procTime =    4.001
          elapsed time():    1.446 secs
          read rate = 6916 objs/s

          min latency = 67 us
          max latency = 7206 us
          avg latency = 400 us
          2ms+ latency count = 10

          latency hist[0..1999] = 9990
          latency hist[2000..3999] = 6
          latency hist[4000..5999] = 3
          latency hist[6000..7999] = 1
          latency hist[8000..9999] = 0
          latency hist[10000..11999] = 0
          latency hist[12000..13999] = 0
          latency hist[14000..15999] = 0
          latency hist[16000..17999] = 0
          latency hist[18000..19999] = 0
          latency hist[20000..21999] = 0
          latency hist[22000..23999] = 0
          latency hist[24000..25999] = 0
          latency hist[26000..27999] = 0
          latency hist[28000..29999] = 0
          latency hist[30000..31999] = 0
          latency hist[32000..33999] = 0
          latency hist[34000..35999] = 0
          latency hist[36000..37999] = 0
          latency hist[38000..39999] = 0

Data check ...
[ 0]: /vagrant/hpc-clone/rt-data/rt/02/00/00/00000013.dat (287)
[ 1]: /vagrant/hpc-clone/rt-data/rt/00/03/01/00000054.dat (163)
[ 2]: /vagrant/hpc-clone/rt-data/rt/00/01/02/00000009.dat (395)
[ 3]: /vagrant/hpc-clone/rt-data/rt/01/00/00/00000047.dat (299)
[ 4]: /vagrant/hpc-clone/rt-data/rt/02/00/02/00000095.dat (432)
[ 5]: /vagrant/hpc-clone/rt-data/rt/01/00/03/00000002.dat (444)
[ 6]: /vagrant/hpc-clone/rt-data/rt/01/02/00/00000037.dat (395)
[ 7]: /vagrant/hpc-clone/rt-data/rt/01/01/02/00000093.dat (326)
[ 8]: /vagrant/hpc-clone/rt-data/rt/01/01/00/00000019.dat (192)
[ 9]: /vagrant/hpc-clone/rt-data/rt/03/00/03/00000028.dat (259)
[10]: /vagrant/hpc-clone/rt-data/rt/02/00/02/00000076.dat (197)
[11]: /vagrant/hpc-clone/rt-data/rt/02/03/03/00000056.dat (185)
[12]: /vagrant/hpc-clone/rt-data/rt/03/00/02/00000051.dat (320)
[13]: /vagrant/hpc-clone/rt-data/rt/00/03/00/00000098.dat (249)
[14]: /vagrant/hpc-clone/rt-data/rt/02/02/03/00000057.dat (342)
Freeing thread structs
Freeing global arrays
Done ... exiting
```