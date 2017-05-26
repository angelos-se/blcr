#!/bin/sh
#
# Test of checkpoint/restart of an sml script.
# Note that the outer shell script is bash

# First ensure we have sml
sml -h >/dev/null || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[123] .Context[123].tmp 2>/dev/null
bash <<-'__OUTER__'
	exec 2>/dev/null # Drop job control message(s) from the outer script
	echo '#ST_ALARM:120'
	echo '#ST_IGNORE:^Standard ML of New Jersey'
	echo '#ST_IGNORE:\[library .* is stable\]'
	echo '#ST_IGNORE:\[autoloading.*\]'
	$cr_run sml 2>&1 <<'__INNER__'
		(* Quiet most internal messages *)
		Control.Print.out := {say=fn _=>(), flush=fn()=>()};
		SMLofNJ.Internals.GC.messages false;
		(* Lots of functions to build up *)
		val pidToInt = (SysWord.toInt o Posix.Process.pidToWord);
		fun waitpid (i:int) = (Posix.Process.waitpid ((Posix.Process.W_CHILD o Posix.Process.wordToPid o SysWord.fromInt) i, []));
		val sleep = (ignore o Posix.Process.sleep o Time.fromSeconds o Int.toLarge);
		fun fork () = (case (Posix.Process.fork ()) of
				  NONE => 0
				| SOME pid => (pidToInt pid));
		fun exit (i:int) = ignore(OS.Process.exit (if (i = 0) then OS.Process.success else OS.Process.failure));
		fun spawn (cmd:string list) =
		  let
		    val child = fork();
		  in
		    if (child = 0) then (
			Posix.Process.exec ((List.hd cmd), cmd);
			exit 1; (* not reached *)
			0
		    ) else child
		  end
		fun println (s:string) = print(s ^ "\n");
		val itoa = Int.toString;
		
		val self = pidToInt(Posix.ProcEnv.getpid());
		local
		    fun getenv (key:string) = (case Posix.ProcEnv.getenv(key) of NONE => "UNDEF" | SOME value => value);
		    val chkptCmd = getenv "cr_checkpoint";
		    val rstrtCmd = getenv "cr_restart"
		in
		    fun checkpoint (file:string, pid:int) = (
			spawn [chkptCmd, ("--file=" ^ file), "--term", "--tree", (itoa pid)];
			if (pid <> self) then ignore(waitpid pid) else ();
			sleep 1
		    );
		    fun restart (file:string) =
			((spawn [rstrtCmd, file]) before (sleep 5))
		end;
		(* Fire up a child who counts for us *)
		val pid = fork();
		if (pid = 0) then (
		    let
			fun say i =  (println ((itoa i) ^ " Hello"); sleep 1);
			fun hello 0 = say 0
		 	  | hello i = (hello(i - 1); say i)
		    in
			hello 15;
			exit 0
		    end )
		else sleep 3;
		(* Checkpoint and restart repeatedly *)
		println "# Checkpoint original child";
		checkpoint("Context1", pid);
		println "# Restart 1";
		val pid = restart("Context1");
		println "# Checkpoint restarted child";
		checkpoint("Context2", pid);
		println "# Restart^2";
		val pid = restart("Context2");
		println "# Checkpoint self";
		checkpoint("Context3", self);
		waitpid pid;
		println "16 DONE";
		exit 0
		__INNER__
	exec 2>&1
	sleep 3
	echo "# Restart interpreter"
	$cr_restart Context3
	__OUTER__
\rm -f Context[123] .Context[123].tmp 2>/dev/null
