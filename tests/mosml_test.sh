#!/bin/sh
#
# Test of checkpoint/restart of a mosml script.
# Note that the outer script is bourne shell

# Check for mosml
(mosml -quietdec </dev/null >/dev/null) || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[12] .Context[12].tmp 2>/dev/null
echo '#ST_ALARM:120'
exec 2>/dev/null # Drop job control message(s)
$cr_run mosml -quietdec <<-'__EOF__' 2>&1 &
	load "Int";
	load "Process";
	let
	    val itoa = Int.toString;
	    fun sleep x = Process.system("sleep " ^ (itoa x))
	    fun greet x = (if x > 0 then ignore (greet(x - 1); sleep 1) else ();
			   print ((itoa x) ^ " Hello\n"))
	in
	    greet 9
	end;
	quit()
	__EOF__
sleep 3; echo "# Checkpoint original child"
$cr_checkpoint --file=Context1 --tree --term $! 2>&1
wait; sleep 1; echo "# Restart 1"
$cr_restart Context1 2>&1 &
sleep 3; echo "# Checkpoint restarted child"
$cr_checkpoint --file=Context2 --tree --term $! 2>&1
wait; sleep 1; echo "# Restart^2"
exec 2>&1
$cr_restart Context2
echo "10 DONE"
\rm -f Context[12] .Context[12].tmp 2>/dev/null
