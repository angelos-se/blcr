#!/bin/sh
set -e
. ${cr_testsdir:-`dirname $0`}/shellinit
context=Context1
code=42
trap "\rm -f $context tstmap1 tstmap2 2>/dev/null" 0
#
run_one () {
  set +e
  ${cr_restart} $1 "exit $code" $2
  result=$?
  if [ $result != $code ]; then
    echo "Test of $1 exited with $result when expecting $code"
    exit 1
  fi
  set -e
}
#
# Test group I: Bogus args
#
# Case I.1: bad arguments (args failure)
run_one --run-on-fail-args --bad-argument
# Case I.2: bad file (permanent failure)
run_one --run-on-fail-perm /dev/null
#
# Test group II: Context from save_aux with 1 mmaped file:
#
\rm -f tstmap1
${cr_run} -- ${cr_testsdir}/save_aux "--file $context --clobber" P
# Case II.1: expect success
${cr_restart} $context
# Case II.2: missing file (environmental failure)
\mv -f tstmap1 tstmap2
run_one --run-on-fail-env $context
# Case II.3: expect success to produce special value
\mv -f tstmap2 tstmap1
run_one --run-on-success $context
#
# Test group III: Context from pause
#
\rm -f $context
${cr_run} -- ${cr_testsdir}/pause >/dev/null 2>/dev/null &
pid=$!
sleep 1
${cr_checkpoint} --file $context --clobber --pid $pid
# Case III.1: pid conflict because still running (temporary failure)
run_one --run-on-fail-temp $context
# Cleanup
exec 2>/dev/null # Drop job control message(s)
set +e
kill $pid
wait $pid
exit 0
