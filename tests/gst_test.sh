#!/bin/bash
#
# Test of checkpoint/restart of GNU Smalltalk (gst)
# Note that the outer script is bash (no good w/ dash)

# Check for gst
(gst -v >/dev/null) || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit
filelist="gst.im Context[12] .Context[12].tmp"

\rm -f $tmpfiles 2>/dev/null
trap "\rm -f $filelist 2>/dev/null" 0

exec 2>/dev/null # Drop job control message(s)
echo '#ST_ALARM:120'
$cr_run gst -Q <<-'__EOF__' 2>&1
	"First we need some things from libc"
	DLD addLibrary: 'libc'!
	SystemDictionary defineCFunc: 'getpid'
          withSelectorArgs: 'getpid'
          returning: #int
          args: #()!
	SystemDictionary defineCFunc: 'getppid'
          withSelectorArgs: 'getppid'
          returning: #int
          args: #()!
	| sema checkpoint |
	sema := (Semaphore new).
	checkpoint := "Block/closure used as a function"
	    [ :file :pid | | s cmd |
		s := WriteStream on: (String new).
		s nextPutAll: (Smalltalk getenv: 'cr_checkpoint'),
				' --file=', file,
				' --tree --kill ', pid printString.
		cmd := s contents.
		stdout display: '# ', cmd; nl; flush.
		Smalltalk system: cmd.
	    ].
	"Create a smalltalk thread (not a process) to count for us"
	    [   0 to: 9 do: [ :i |
		    stdout display: i printString, ' Hello'; nl; flush.
		    (Delay forSeconds:1) wait.
	        ].
	        sema signal.
	    ] fork.
	(Delay forSeconds:3) wait.
	checkpoint value: 'Context1' value: (Smalltalk getpid).
	(Delay forSeconds:3) wait.
	checkpoint value: 'Context2' value: (Smalltalk getppid).
	sema wait.
	stdout display: '10 Goodbye'; nl; flush.
	!
	__EOF__
sleep 1; echo "# Restart 1"
$cr_restart Context1 2>&1
sleep 1; echo "# Restart^2"
exec 2>&1 # No more job-control obituaries to supress
$cr_restart Context2
echo "11 DONE"
