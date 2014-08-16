Setup:

  $ . $TESTDIR/setup.sh

Markup stripping:

  $ singleshot 'foo <b>bar</b>'
  [cram] foo: bar 

Raw mode:

  $ run -1 -r; send 'foo <b>bar</b>'; fin
  [cram] foo: <b>bar</b> 
