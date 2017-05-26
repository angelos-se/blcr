#!/bin/sh
#
# Test of checkpoint/restart of a zsh script.
# Note that the outer script is bash

# Check for zsh
zsh -c true || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run zsh 2>&1 <<-'__INNER__'
		zsh -c 'for ((i=0; $i<=15; ++i)); do echo $i Hello; sleep 1; done' &
		pid=$!
		sleep 3
		echo "# Checkpoint original child"
		$cr_checkpoint --file=Context1 --tree --term $pid
		wait
		sleep 1
		echo "# Restart 1"
		$cr_restart Context1 &
		pid=$!
		sleep 3
		echo "# Checkpoint restarted child"
		$cr_checkpoint --file=Context2 --tree --term $pid
		wait
		sleep 1
		echo "# Restart^2"
		$cr_restart Context2 &
		pid=$!
		sleep 3
		echo "# Checkpoint self"
		$cr_checkpoint --file=Context3 --term --tree $$
		wait
		echo "16 DONE"
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart shell"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
