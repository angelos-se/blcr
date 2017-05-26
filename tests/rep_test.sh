#!/bin/sh

# First check that we *have* rep
rep -q || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
exec 2>/dev/null # To drop job control messages
echo '#ST_ALARM:120'
$cr_run rep --batch --load /proc/self/fd/0 <<-EOF 2>&1 &
	(defun greet (n)
	    (flush-file (format standard-output "%d Hello\n" n))
	    (sleep-for 1)
	    (if (< n 9) (greet (+ n 1))))
	(greet 0)
	(flush-file (format standard-output "# Checkpoint self\n"))
	(system
	    (format nil "%s --file=Context3 --term --tree %d"
		(getenv "cr_checkpoint") (process-id)))
	(format standard-output "11 DONE\n")
EOF
pid=$!
sleep 3
echo "# Checkpoint original child"
$cr_checkpoint --file=Context1 --term $pid 2>&1
wait
sleep 1
echo "# Restart 1"
$cr_restart Context1 2>&1 &
pid=$!
sleep 3
echo "# Checkpoint restarted child"
$cr_checkpoint --file=Context2 --term --tree $pid 2>&1
wait
sleep 1
echo "# Restart^2"
$cr_restart Context2 2>&1 &
pid=$!
wait
echo "10 Restart interpreter"
$cr_restart Context3 2>&1
\rm -f Context[123] .Context[123].tmp 2>/dev/null
