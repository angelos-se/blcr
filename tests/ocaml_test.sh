#!/bin/sh
#
# Test of checkpoint/restart of an ocaml script.
# Note that the outer shell script is bash

# First ensure we have ocaml
ocaml -help >/dev/null || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	$cr_run ocaml /proc/self/fd/0 2>&1 <<'__INNER__'
		#load "unix.cma";;
		let chkpt_cmd = Unix.getenv "cr_checkpoint";;
		let rstrt_cmd = Unix.getenv "cr_restart";;
		let self = Unix.getpid ();;
		let checkpoint f p =
		  let cmd = (chkpt_cmd ^ " --file=" ^ f ^ " --tree --term " ^ (string_of_int p)) in
		    ignore (Unix.system cmd);
		  if p != self then ignore (Unix.waitpid [] p);
		  Unix.sleep 1;;
		let restart f =
		  let pid = Unix.fork () in (
		    if pid == 0 then ignore (Unix.execv rstrt_cmd [| rstrt_cmd; f |]);
		    Unix.sleep 3;
		    pid);;
		let child = Unix.fork ();;
		if child == 0 then (
		  for i = 0 to 15 do
		    print_endline ((string_of_int i) ^ " Hello");
		    Unix.sleep 1;
		  done;
		  exit 0 )
		else 
		  Unix.sleep 3;;
		print_endline "# Checkpoint original child";;
		checkpoint "Context1" child;;
		print_endline "# Restart 1";;
		let pid = restart "Context1";;
		print_endline "# Checkpoint restarted child";;
		checkpoint "Context2" pid;;
		print_endline "# Restart^2";;
		let pid = restart "Context2";;
		print_endline "# Checkpoint self";;
		checkpoint "Context3" self;;
		Unix.waitpid [] pid;;
		print_endline "16 DONE";;
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart interpreter"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
