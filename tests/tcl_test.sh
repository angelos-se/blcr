#!/bin/sh
#
# Test of checkpoint/restart of a tcl script.
# Note that the outer shell script is bash

# Check for tclsh
(echo exit 0 | tclsh) || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run tclsh 2>&1 <<-'__INNER__'
		set chkpt_cmd $env(cr_checkpoint)
		set rstrt_cmd $env(cr_restart)
		proc waitfor {pid} { while { ! [ catch { exec kill -s 0 $pid } msg ] } { after 1000 } }
		proc checkpoint {file child} {
		    global chkpt_cmd
		    exec -- $chkpt_cmd "--file=$file" --tree --term $child
		    if { $child != [ pid ] } { waitfor $child }
		    after 1000
		}
		proc restart {file} {
		    global rstrt_cmd
		    set child [ exec -- $rstrt_cmd $file & ]
		    after 3000
		    return $child
		}
		proc echo {msg} { puts $msg; flush stdout }
		flush stderr
		flush stdout
		set cmd {for {set i 0} {$i<16} {incr i} {\
				puts "$i Hello";\
				flush stdout;\
				after 1000}}
		set child [ exec [info nameofexecutable] << "$cmd" & ]
		after 3000
		echo "# Checkpoint original child"
		checkpoint "Context1" $child
		echo "# Restart 1"
		set $child [ restart "Context1" ]
		echo "# Checkpoint restarted child"
		checkpoint "Context2" $child
		echo "# Restart^2"
		set $child [ restart "Context2" ]
		echo "# Checkpoint self"
		checkpoint "Context3" [ pid ]
		waitfor $child
		echo "16 DONE"
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart interpreter"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
