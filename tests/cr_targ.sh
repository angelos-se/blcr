#!/bin/sh
. ${cr_testsdir:-`dirname $0`}/shellinit
#
${cr_run} -- ${cr_testsdir}/pause >/dev/null 2>/dev/null &
pid=$!
sleep 1
\rm -f context.$pid 2>/dev/null
trap "\rm -f context.$pid 2>/dev/null" 0
${cr_checkpoint} --clobber --quiet $pid
result=$?
if test $result = 0; then
  : # OK
else
  echo "Checkpoint unexpectedly failed with exit code $result" >&2
  exit 1
fi
exec 2>/dev/null # Drop job control message(s)
kill $pid
wait $pid
exit 0
