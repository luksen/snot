Setup:

  $ . $TESTDIR/setup.sh

No delay:

  $ run -d 0 >/dev/null ; send -t 1000 foo bar; sleep 3; fin
