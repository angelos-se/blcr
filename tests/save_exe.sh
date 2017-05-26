#!/bin/sh
set -e
. ${cr_testsdir:-`dirname $0`}/shellinit
context=Context1
#
cp -f ${cr_testsdir}/save_aux tstexe
trap "\rm -f $context tstexe 2>/dev/null" 0
${cr_run} ./tstexe "--file $context --clobber --save-exe"
# Unlink and overwrite exec
rm -f tstexe
cp /dev/null tstexe
${cr_restart} $context
