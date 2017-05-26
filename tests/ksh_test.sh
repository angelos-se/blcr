#!/bin/sh
#
# Test of checkpoint/restart of a ksh script.
# Note that the outer script is bash

# Check for ksh
ksh -c true || exit 77

# NOTE: Unlike most other shells test cases, this one uses the
# --kill flag to cr_checkpoint.  This is because
# ksh tends to exit when one of its subshells is terminated.

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run ksh 2>&1 <<-'__INNER__'
		echo '#ST_IGNORE:^\[1\]' # Tell seq_wrapper to ignore job-control msgs
		echo '#ST_IGNORE:^Killed' # Tell seq_wrapper to ignore job-control msgs
		echo '#ST_IGNORE:^[[:space:]]*$' # ...and the whitespace lines that may accompany them.
		ksh -c 'for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do echo $i Hello; sleep 1; done' &
		pid=$!
		sleep 3
		echo "# Checkpoint original child"
		$cr_checkpoint --file=Context1 --tree --kill $pid
		wait
		sleep 1
		echo "# Restart 1"
		$cr_restart Context1 &
		pid=$!
		sleep 3
		echo "# Checkpoint restarted child"
		$cr_checkpoint --file=Context2 --tree --kill $pid
		wait
		sleep 1
		echo "# Restart^2"
		$cr_restart Context2 &
		pid=$!
		sleep 3
		echo "# Checkpoint self"
		$cr_checkpoint --file=Context3 --kill --tree $$
		wait
		echo "16 DONE"
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart shell"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
