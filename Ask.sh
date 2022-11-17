#!/bin/sh
#
# Ask a question and return yes or no

#Ask()
#{
	prompt=$1
	default=$2
	if [ "`echo '\c'`" = "" ]; then
		echo "$prompt [$default] \c"	>/dev/tty
	else
		echo -n "$prompt [$default] "	>/dev/tty
	fi
	read answer
	if [ "$answer" = "" ]; then
		echo $default
	else
		echo $answer
	fi
#}
