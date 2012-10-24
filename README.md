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
