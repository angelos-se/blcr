#!/bin/sh

# First check that we *have* ruby
ruby -e '' || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__EOF__'
        exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run ruby -e '
		include Process
		STDOUT.sync=true
		$chkpt_cmd = ENV['\''cr_checkpoint'\'']
		$rstrt_cmd = ENV['\''cr_restart'\'']
		def checkpoint(file,pid)
			system "#{$chkpt_cmd} --file=#{file} --kill --tree #{pid}"
			waitpid(pid) if pid != $$
			sleep 1
		end
		def restart(file)
			pid = fork { exec "#{$rstrt_cmd} #{file}" }
			sleep 3
			return pid
		end
		pid = fork { 0.upto(15) {|n| print "#{n} Hello\n"; sleep 1} }
		sleep 3
		print "# Checkpoint original child\n"
		checkpoint("Context1",pid)
		print "# Restart 1\n"
		pid = restart "Context1"
		print "# Checkpoint restarted child\n"
		checkpoint("Context2",pid)
		print "# Restart^2\n"
		pid = restart "Context2"
		print "# Checkpoint self\n"
		checkpoint("Context3",$$)
		waitpid pid
                print "16 DONE\n"
                exit 0' 2>&1
        exec 2>&1
        sleep 3
        echo "# Restart interpreter"
        $cr_restart Context3
__EOF__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
