Setup:

  $ . $TESTDIR/setup.sh
  $ close() {
  >   dbus-send --print-reply=literal --type=method_call \
  >   --dest=org.freedesktop.Notifications \
  >   /org/freedesktop/Notifications \
  >   org.freedesktop.Notifications.CloseNotification \
  >   int32:$1
  > }

Close notification:

  $ run -f '%s'; send a; sleep 1; close 2; sleep 1; fin
  a
  a

Close notification with 0:

  $ run -f '%s'; send a; sleep 1; close 0; sleep 1; fin
  a
  a

Close middle notification:

  $ run -f '%s'; send a; send b; send c; sleep 1; close 3; sleep 5; fin
  a
  a
  a
  c
  c
  c
