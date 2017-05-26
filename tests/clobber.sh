#!/bin/sh
# Test for the atomicity flags to cr_checkpoint
set -e
. ${cr_testsdir:-`dirname $0`}/shellinit
context=Context1
backup=Context2
stamp1=tststamp1
stamp2=tststamp2
trap "\rm -f $context $backup $stamp1 $stamp2 2>/dev/null" 0
#
stamp_it () {
  if [ -e $1 ]; then
    touch -r $1 $2
  else
    touch $2
  fi
  sleep 1 # ensure files have different ctimes
}
test_proc () {
  stamp_it $context $stamp1
  set +e
  ${cr_run} -- ${cr_testsdir}/save_aux "--file $context $1" 2>/dev/null
  result=$?
  set -e
  if [ $result != $2 ]; then
    echo "Test of '$1' exited with $result when expecting $2"
    exit 1
  fi
  if [ $2 = 0 -a \! $context -nt $stamp1 ]; then
    echo "context unexpectedly appears exchanged"
    exit 1
  fi
}
test_init () {
  stamp_it $context $stamp1
  set +e
  ${cr_checkpoint} --file $context $1 1 2>/dev/null
  result=$?
  set -e
  if [ $result = 0 ]; then
    echo "checkpoint of '$1 1' unexpectedly succeeded"
    exit 1
  fi
  if [ \! -e $context ]; then
    echo "checkpoint of '$1 1' unexpectedly removed context file"
    exit 1
  fi
  if [ $context -nt $stamp1 ]; then
    echo "checkpoint of '$1 1' unexpectedly changed context file"
    exit 1
  fi
}
#
\rm -f $context $backup
test_proc --noclobber 0 # Expect pass, no conflicting file
test_proc --noclobber 1 # Expect fail since file exsits
test_proc --clobber 0   # Expect pass & replacing
test_init --atomic
test_proc --atomic 0    # Expect pass & replace
test_init --backup=$backup
if [ -e $backup ]; then
  echo "backup file created unexpectedly"
  exit 1
fi
# each one should clobber the previous 
\rm -f $stamp2
for i in 1 2 3; do
  test_proc --backup=$backup 0
  if [ \! -e $backup ]; then
    echo "backup file unexpectedly missing"
    exit 1
  fi
  if [ $backup -nt $context ]; then
    echo "backup file and context appear exchanged"
    exit 1
  fi
  if [ -e $stamp2 -a \! $backup -nt $stamp2 ]; then
    echo "backup file unexpectedly appears too old at $i"
    exit 1
  fi
  touch -r $backup $stamp2
done
