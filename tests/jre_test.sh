#!/bin/sh

# Test JRE from Sun or a work-alike.
#
# NOTE: Sun's java uses signal handlers to remove the shared memory file
# /tmp/hsperfdata_<user>/<pid> on termination, thus preventing restart.
# We have two options.  One is the '-Xrs' option to disable the signal
# handlers, and the other is to send SIGKILL.
# Currently we use -Xrs, which is silently ignored by both IBM's java
# and by gij.   Thus Sun's, IBM's and the gcj JREs all work the same.
#
# Tested against the following RPMs:
#   java-1_5_0-sun-1.5.0_10-0.1
#   ibm-java2-i386-sdk-5.0-5.0
#   java-1.4.2-gcj-compat
#   java-sablevm
#

# First check that we *have* java
(java -version || java -help) >/dev/null 2>&1 || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit
top_srcdir=${top_srcdir:-..}
filelist="Context[12] .Context[12].tmp"

\rm -f $filelist 2>/dev/null
trap "\rm -f $filelist 2>/dev/null" 0

exec 2>/dev/null # To drop job control messages
echo '#ST_ALARM:120'
echo '#ST_IGNORE:^Warning: .*Ignoring\.$'
echo '#ST_IGNORE:^Warning: -Xrs option not implemented'
$cr_run java -cp $top_srcdir/tests -Xrs CountingApp 2>&1 &
pid=$!
sleep 3
echo "# Checkpoint 1"
$cr_checkpoint --file=Context1 --term $pid 2>&1
wait
sleep 1
echo "# Restart 1"
$cr_restart Context1 2>&1 &
pid=$!
sleep 3
echo "# Checkpoint restarted child"
$cr_checkpoint --file=Context2 --term --tree $pid 2>&1
wait
sleep 1
echo "# Restart^2"
$cr_restart Context2 2>&1 &
pid=$!
wait
echo "10 DONE"
