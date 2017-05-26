#!/bin/sh
set -e
. ${cr_testsdir:-`dirname $0`}/shellinit
context=Context1
#
cp -f ${cr_testsdir}/save_aux tstexe
trap "\rm -f $context tstexe tstmap[12] 2>/dev/null" 0
rm -f tstmap[12]
${cr_run} ./tstexe "--file $context --clobber --save-private" P
# Unlink and overwrite exec; unlink mmap()ed file
rm -f tstexe tstmap[12]
cp /dev/null tstexe
${cr_restart} $context
