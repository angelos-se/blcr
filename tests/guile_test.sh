#!/bin/sh

# First check that we *have* guile
guile -c '(quit)' || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
exec 2>/dev/null # To drop job control messages
echo '#ST_ALARM:120'

# Alas, (format) is not portable across guile versions
$cr_run guile -c '
	(define greet
	  (lambda (n)
	    (if (< n 10)
	        (begin
	          (display n)(display " Hello\n")(force-output)
	          (sleep 1)
	          (greet (+ n 1))))))
	(greet 0)
	(display "# Checkpoint self\n")(force-output)
	(system (string-append
		  (getenv "cr_checkpoint")
		  " --file=Context3 --term --tree "
		  (number->string (getpid))))
	(display "11 DONE\n")
	(quit)' 2>&1 &
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
