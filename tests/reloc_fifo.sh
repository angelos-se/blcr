#!/bin/sh
set -e
. ${cr_testsdir:-`dirname $0`}/shellinit
context=`pwd`/Context1
#
TMPDIR=`cd ${TMPDIR:-/tmp} && $cr_pwd`
MY_TMPDIR1=`mktemp -d ${TMPDIR}/blcr_reloc.XXXXXX`
MY_TMPDIR2=`mktemp -d ${TMPDIR}/blcr_reloc.XXXXXX`
trap "\rm -rf ${MY_TMPDIR1} ${MY_TMPDIR2} $context 2>/dev/null" 0
cp -f ${cr_testsdir}/reloc_aux ${MY_TMPDIR1}/reloc_exe
${cr_run} ${MY_TMPDIR1}/reloc_exe ${MY_TMPDIR1} "$context --clobber"
# Move the open() fifo
mv -f ${MY_TMPDIR1}/reloc_fifo ${MY_TMPDIR2}/
${cr_restart} --relocate ${MY_TMPDIR1}/reloc_fifo=${MY_TMPDIR2}/reloc_fifo $context
