#!/bin/sh
set -e
. ${cr_testsdir:-`dirname $0`}/shellinit
#
${cr_run} ${cr_testsdir}/bug2003_aux &
pid=$!
sleep 2
${cr_checkpoint} --clobber $pid
sleep 2
${cr_restart} context.$pid
rm -f context.$pid
