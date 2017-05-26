#!/bin/sh
#
# Test of checkpoint/restart of a php script.
# Note that the outer script is bourne shell

# Check for php
(echo | php >/dev/null) || exit 77

. ${cr_testsdir:-`dirname $0`}/shellinit

\rm -f Context[12] .Context[12].tmp 2>/dev/null
echo '#ST_IGNORE:^X-[-a-zA-Z]+:'
echo '#ST_IGNORE:^Content-type:'
echo '#ST_IGNORE:^[[:space:]]*$'
echo '#ST_ALARM:120'
exec 2>/dev/null # Drop job control message(s)
$cr_run php <<-'__EOF__' 2>&1 &
	<?php for ($i=0; $i<10; $i++) { echo $i . " Hello\n" . `sleep 1`; } ?>
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
