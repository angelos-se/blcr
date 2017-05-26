#!/bin/sh
# Test gij (the Java bytecode interpreter from the GNU Compiler Collection)

# First check that we *have* gij
gij --version >/dev/null || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit
CLASSPATH=${top_srcdir:-..}/tests
export CLASSPATH
filelist="Context[12] .Context[12].tmp"

\rm -f $filelist 2>/dev/null
trap "\rm -f $filelist 2>/dev/null" 0

exec 2>/dev/null # To drop job control messages
echo '#ST_ALARM:120'
$cr_run gij CountingApp 2>&1 &
pid=$!
sleep 3
echo "# Checkpoint 1"
$cr_checkpoint --file=Context1 --term $pid 2>&1
wait
sleep 3
echo "# Restart 1"
$cr_restart Context1 2>&1 &
pid=$!
sleep 3
echo "# Checkpoint restarted child"
$cr_checkpoint --file=Context2 --term --tree $pid 2>&1
wait
sleep 3
echo "# Restart^2"
$cr_restart Context2 2>&1 &
pid=$!
wait
echo "10 DONE"
