# All cram tests should use this. Make sure that "ag" runs the version
# of ag we just built, and make the output really simple.

alias snot="$TESTDIR/../snot"

run() {
	snot $@ & sleep 1
}

send() {
	notify-send -a cram $@
}

fin() {
	killall -TERM snot
}

singleshot() {
	run -1
	send $@
	fin
}
