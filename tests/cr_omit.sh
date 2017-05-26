#!/bin/bash
. ${cr_testsdir:-`dirname $0`}/shellinit
#
${cr_run} --omit ${cr_testsdir}/pause >/dev/null 2>/dev/null &
pid=$!
sleep 1
ESRCH=3
${cr_checkpoint} --clobber --term $pid 2>/dev/null
result=$?
if test $result = $ESRCH; then
  : # OK
elif test $result = 0; then
  echo "Checkpoint suceeded unexpectedly" >&2
  exit 1
else
  echo "Checkpoint unexpectedly failed with exit code $result" >&2
  exit 1
fi
exec 2>/dev/null # Drop job control message(s)
kill $pid
result=$?
wait $pid
exec 2>&1
if test $result = 0; then
  : # OK
else
  echo "Target process disappeared unexpectedly" >&2
  exit 1
fi
exit 0
