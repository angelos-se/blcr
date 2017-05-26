#!/bin/sh
#
# Test of checkpoint/restart of a csh script.
# Note that "outermost" shell is bash

# First make certain we *have* a csh
# Exit code 77 tells automake "test skipped"
csh -c true || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run csh 2>&1 <<-'__INNER__'
		echo '#ST_IGNORE:^\[1\]' # Tell seq_wrapper to ignore job-control msgs
		echo '#ST_IGNORE:^[[:space:]]*$' # ...and the whitespace lines that may accompany them.
		csh <<'__EOF__' &
		    foreach i (0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15)
   	 	    	echo $i Hello
   	 	    	sleep 1
		    end
		'__EOF__'
		set pid=$!
		sleep 3
		echo "# Checkpoint $pid"
		$cr_checkpoint --file=Context1 --tree --term $pid
		sleep 2
		echo "# Restart $pid"
		$cr_restart Context1 &
		set pid=$!
		sleep 4
		echo "# Checkpoint cr_restart of $pid"
		$cr_checkpoint --file=Context2 --tree --term $pid
		sleep 2
		echo "# Restart cr_restart of $pid"
		$cr_restart Context2 &
		set pid=$!
		sleep 4
		echo "# Checkpoint self"
		$cr_checkpoint --file=Context3 --tree --term $$
		# Work around a tcsh bug (BLCR bug 2028) that prevents using wait
		while { kill -0 $pid >& /dev/null }
		    sleep 1
		end
		echo "16 DONE"
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart of shell"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
