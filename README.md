## snot - simple notification

Text based notification server for libnotify. 

# Options

`-t or --timeout` Overwrite the standard timeout for notifications without 
    specified timeout

`-f or --format` Define the output format as described in the next paragraph.

# Format

Snot allows you to define the output string on the command line. The following
substitutions are done:

`%a` application name

`%s` notification summary

`%b` notification body

`%q` size of queue (not yet displayed notifications)

Example:

    snot -f "%a: _%s_ %b [%q]"

# close Notification
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


# Capabilities
    name                planned     implemented
    -------------------------------------------
    "action-icons"      no          no
    "actions"           yes         no
    "body"              yes         yes
    "body-hyperlinks"   no          no
    "body-images"       no          no
    "body-markup"       maybe       no
    "icon-multi"        no          no
    "icon-static"       no          no
    "persistence"       maybe       no
    "sound"             maybe       no
