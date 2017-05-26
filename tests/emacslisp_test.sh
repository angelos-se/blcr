#!/bin/sh

# First check that we *have* emacs
emacs -nw -batch 2>/dev/null || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
exec 2>/dev/null # To drop job control messages
echo '#ST_ALARM:120'
$cr_run_lb emacs -nw -q -batch \
 -eval '(defun greet (n)
		(princ (format "%d Hello\n" n))
		(sleep-for 1)
		(if (< n 9) (greet (+ n 1))))' \
 -eval '(defun getpid-ascii () ;; You would think emacslisp would implement getpid()
		(car (split-string  ;; Extract 1st whitespace-delimited field of /proc/sef/stat
			(with-temp-buffer
    				(insert-file-contents-literally "/proc/self/stat")
    				(buffer-string)))))' \
 -eval '(progn
		(greet 0)
		(princ "# Checkpointing self\n")
		(call-process (getenv "cr_checkpoint")
			nil nil nil
			"--file=Context3" "--term" "--tree" (getpid-ascii))
		(princ "10 DONE\n"))' \
  -kill &
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
wait
echo "# Restart interpreter"
sleep 2
$cr_restart Context3 2>&1
\rm -f Context[123] .Context[123].tmp 2>/dev/null
