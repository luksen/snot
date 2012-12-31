# snot - simple notification

Text based notification server for libnotify directly based on libdbus. 

See [Todo](#todo) and below for information on the current limits of this
implementation.

## Options

`-t or --timeout` Overwrite the standard timeout for notifications without 
    specified timeout

`-f or --format` Define the output format as described in the next paragraph.

`-r or --raw` Don't remove markup.

## Format

Snot allows you to define the output string on the command line. The following
substitutions are done:

`%a` application name

`%s` notification summary

`%b` notification body

`%q` size of queue (not yet displayed notifications)

`%(x...)` only display ... if x evaluates to true; x may be one of the above

Example:

    snot -f "%a: _%s_ %b %(qyou have %q more notifications)"

## close Notification
As per the Desktop Notification Specification it's possible to remove a
notification from the queue by calling the DBus method CloseNotification, e.g.
like this:

    dbus-send --print-reply=literal --type=method_call \
    --dest=org.freedesktop.Notifications \
    /org/freedesktop/Notifications \
    org.freedesktop.Notifications.CloseNotification \
    int32:<id>

Specifying 0 as id will close the head of the queue. This is an extension to the
specification. This is useful for non-expiring notifications or notifications
with a long timeout.

## Todo

 - honor replaces\_ip
 - handle urgency levels


## Capabilities
    name                planned     implemented
    -------------------------------------------
    "action-icons"      no          no
    "actions"           maybe       no
    "body"              yes         yes
    "body-hyperlinks"   no          no
    "body-images"       no          no
    "body-markup"       maybe       no
    "icon-multi"        no          no
    "icon-static"       no          no
    "persistence"       maybe       no
    "sound"             maybe       no          (through external program call)
