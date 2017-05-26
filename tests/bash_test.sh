#!/bin/sh
#
# Test of checkpoint/restart of a bash script.
# The two inner-most shells here are the actual tested code.
# The first nested shell checkpoints and restarts the
# inner-most and then finally checkpoints itself.
# Outer-most shell is just to discard the stderr generated
# when its child dies from a self-inflicted SIGTERM.
#
# In addition to checkpointing and restarting bash, this
# test also tests checkpoint/restart of the cr_restart
# and cr_checkpoint executables.  The cr_restart command is
# tested both as "root" of a checkpoint (in Context2)
# and as child of the shell (in Context3).  The cr_checkpoint
# command (for Context3) checkpoints itself.
#
. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job-control messages
	echo '#ST_ALARM:120'
	$cr_run bash <<-'__INNER__'
		bash -c 'for ((i=0; $i<=15; ++i)); do echo $i Hello; sleep 1; done' &
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
