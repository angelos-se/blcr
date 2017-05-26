#!/bin/bash
#
# Test of checkpoint/restart of a python script.
# Note that the outer shell script is bash

# First ensure we have python
python -c 1 || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run python 2>&1 <<'__INNER__'
		import os, sys, time, signal
		chkpt_cmd = os.environ.get("cr_checkpoint")
		rstrt_cmd = os.environ.get("cr_restart")
		def checkpoint(file,pid):
		    cmd = ['cr_checkpoint', '--file='+file, '--tree', '--term', str(pid)]
		    #print '# ' + ' '.join(cmd)
		    os.spawnv(os.P_WAIT,chkpt_cmd,cmd)
		    if pid != os.getpid():
		        os.waitpid(pid, 0)
		        time.sleep(1)
		def restart(file):
		    cmd = ['cr_restart', file]
		    #print '# ' + ' '.join(cmd)
		    pid = os.spawnl(os.P_NOWAIT,rstrt_cmd,'cr_restart',file)
		    time.sleep(3)
		    return pid
		pid = os.fork()
		if pid == 0:
		    for i in range(16):
		            print i, 'Hello'
		            time.sleep(1)
		    sys.exit(0)
		time.sleep(3)
		print '# Checkpoint original child'
		checkpoint('Context1',pid)
		print '# Restart 1'
		pid = restart('Context1')
		print '# Checkpoint restarted child'
		checkpoint('Context2',pid)
		print '# Restart^2'
		pid = restart('Context2')
		print '# Checkpoint self'
		checkpoint('Context3',os.getpid())
		os.waitpid(pid, 0)
		print '16 DONE'
		sys.exit(0)
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart interpreter"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
