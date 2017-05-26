#!/bin/sh
# Checks that dlopen("libcr.so") work.
# Includes attempts to trigger bug 2263 (in its orginally reported form)
. ${cr_testsdir:-`dirname $0`}/shellinit
env LD_LIBRARY_PATH=${cr_libpath}${LD_LIBRARY_PATH} ${cr_testsdir}/dlopen_aux || exit $?
${cr_run} ${cr_testsdir}/dlopen_aux || exit $?
${cr_run} --omit ${cr_testsdir}/dlopen_aux || exit $?
