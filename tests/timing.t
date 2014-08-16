Setup:

  $ . $TESTDIR/setup.sh

Different timeout:

  $ run; send -t 5000 foo bar; sleep 5; fin
  [cram] foo: bar 
  [cram] foo: bar 
  [cram] foo: bar 
  [cram] foo: bar 
  [cram] foo: bar 
  

  $ run -t 5000; send foo bar; sleep 5; fin
  [cram] foo: bar 
  [cram] foo: bar 
  [cram] foo: bar 
  [cram] foo: bar 
  [cram] foo: bar 
  

Different delay:

  $ run -d 3000; send foo bar; sleep 4; fin
  [cram] foo: bar 
  

  $ run -d 3000; send -t 6000 foo bar; sleep 6; fin
  [cram] foo: bar 
  [cram] foo: bar 
  
