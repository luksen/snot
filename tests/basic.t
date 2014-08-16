Setup:

  $ . $TESTDIR/setup.sh

Oneline:

  $ singleshot foo bar
  [cram] foo: bar 

No parameters:

  $ run; send foo bar; sleep 3; fin
  [cram] foo: bar 
  [cram] foo: bar 
  [cram] foo: bar 
  
