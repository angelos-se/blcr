#!/bin/sh

# Some clisp versions complain w/ utf8
LANG=C
export LANG

# First check that we *have* clisp
clisp -q -q -x '(quit)' || exit 77

# NOTE: Unlike most other script test cases, this one uses the
# --kill flag to cr_checkpoint.  This is because we've observed
# a bug in signal handling in some clisp releases (BLCR bug 2470).

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
exec 2>/dev/null # To drop job control messages
echo '#ST_IGNORE:^Exiting on signal'
echo '#ST_ALARM:120'
$cr_run clisp -q -q -x '
    (progn
	(defun greet (n)
	    (format t "~D equals ~R~%" n n)
	    (sleep 1)
	    (if (< n 9) (greet (+ n 1))))
	(greet 0)
	(format t "# Checkpointing self~%")
	(ext:execute
	    (ext:getenv "cr_checkpoint")
	    "--file=Context3" "--kill" "--tree"
	    (write-to-string (funcall (or (find-symbol "PROGRAM-ID" :system)
					  (find-symbol "PROCESS-ID" :system)))))
	(format t "11 DONE~%")
	(quit))' 2>&1 &
pid=$!
sleep 3
echo "# Checkpoint original child"
$cr_checkpoint --file=Context1 --kill $pid 2>&1
wait
sleep 1
echo "# Restart 1"
$cr_restart Context1 2>&1 &
pid=$!
sleep 3
echo "# Checkpoint restarted child"
$cr_checkpoint --file=Context2 --kill --tree $pid 2>&1
wait
sleep 1
echo "# Restart^2"
$cr_restart Context2 2>&1 &
pid=$!
wait
echo "10 Restart interpreter"
$cr_restart Context3 2>&1
\rm -f Context[123] .Context[123].tmp 2>/dev/null
