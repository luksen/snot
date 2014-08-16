# All cram tests should use this.

alias snot="$TESTDIR/../snot"

run() {
	snot $@ &
	sleep 1
}

send() {
	notify-send -a cram $@
}

fin() {
	kill -TERM $! 2>/dev/null
	wait $!
}

singleshot() {
	run -1
	send $@
	fin
}
