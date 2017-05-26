#!/bin/sh
#
# Test of checkpoint/restart of a perl script.
# Note that the outer shell script is bash

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run perl 2>&1 <<-'__INNER__'
		my $chkpt_cmd=$ENV{'cr_checkpoint'};
		my $rstrt_cmd=$ENV{'cr_restart'};
		sub checkpoint($$) {
		    my ($file, $pid) = @_;
		    my $cmd = "$chkpt_cmd --file=$file --tree --term $pid";
		    #print "# $cmd\n";
		    system("$cmd");
		    if ($pid != $$) { waitpid($pid, 0); }
		    sleep 1;
		}
		sub restart($) {
		    my ($file) = @_;
		    my $cmd = "$rstrt_cmd $file";
		    #print "# $cmd\n";
		    defined(my $pid = fork()) || die;
		    if (!$pid) { exec($cmd); exit(1); }
		    sleep 3;
		    $pid;
		}
		$|=1;
		defined(my $pid = fork()) || die;
		if (!$pid) { foreach $i (0..15) { print "$i Hello\n"; sleep 1;}; exit 0; }
		sleep 3;
		print "# Checkpoint original child\n";
		checkpoint('Context1',$pid);
		print "# Restart 1\n";
		$pid = restart('Context1');
		print "# Checkpoint restarted child\n";
		checkpoint('Context2',$pid);
		print "# Restart^2\n";
		$pid = restart('Context2');
		print "# Checkpoint self\n";
		checkpoint('Context3',$$);
		waitpid($pid, 0);
		print "16 DONE\n";
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart interpreter"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
