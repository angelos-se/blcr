#!/bin/sh
# Test gcj (the Java native compiler from the GNU Compiler Collection)

# First check that we *have* gcj
gcj --version >/dev/null || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit
top_srcdir=${top_srcdir:-..}
filelist="CountingApp Context[12] .Context[12].tmp"

\rm -f $filelist 2>/dev/null
trap "\rm -f $filelist 2>/dev/null" 0

gcj --main=CountingApp -o CountingApp $top_srcdir/tests/CountingApp.java >/dev/null 2>&1
if [ $? != 0 ]; then
   echo "Warning: gcj failed to compile CountingApp.java" >&2
   exit 77
fi

exec 2>/dev/null # To drop job control messages
echo '#ST_ALARM:120'
$cr_run ./CountingApp 2>&1 &
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
