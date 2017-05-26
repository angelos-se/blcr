#!/bin/sh
. ${cr_testsdir:-`dirname $0`}/shellinit
${cr_run} ${cr_testsdir}/hello >/dev/null 2>/dev/null
