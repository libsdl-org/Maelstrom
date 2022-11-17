
Over the course of developing Maelstrom, it has accumulated quite a few
command line and compile-time options.

Multiplayer Commmand Line Options:
		-- Only supported if network play is compiled in.

	-player N[@host][:port]
			This tells Maelstrom that it is playing a network
			game, and that player N is at host "host"

	-server N@host[:port]
			This option tells Maelstrom to use a network 
			address server at host "host" for a multiplayer
			game with N players.

	-deathmatch [N]	A multiplayer game continues until someone gets 
			N frags.  A frag is 3 shots at a player.


Command Line Options:

	-netscores	This option tells Maelstrom to use the Internet 
			Maelstrom Score Server for the high score list.

	-printscores	This option prints out a list of the current high
			scores.  If used with the -netscores option, it will
			will connect to the Internet Maelstrom Score Server
			and print out the world-wide Maelstrom high scores.

	-display <host:0>
			This option runs Maelstrom on the given X11 display.
			It is disabled if Maelstrom is compiled with 
			-DFORCE_XSHM

	-privatecmap	Runs Maelstrom with a private (custom) colormap.
			This prevents Maelstrom from locking up all of
			the colors on your display.  It is only useful
			on 256 color (pseudo-color) displays.

	-nofade		This option prevents Maelstrom from doing the
			screen fading.  This is useful if you find screen
			fading annoying.

	-gamma [0-8]	Sets the gamma correction level for Maelstrom.
			The higher the gamma correction, the brighter
			Maelstrom will appear on your monitor.
			If no gamma level is given, Maelstrom will print
			the current gamma level saved in your custom
			configuration.
			Once this option is used, the gamma correction 
			level is saved, and this option doesn't need to
			be used later unless you want to change it again.

	-fullscreen	This option puts a big black border around the
			Maelstrom screen, and centers Maelstrom within it.
			This help create a "full screen" effect on large
			displays.

	-version	This option prints the version of the Maelstrom binary.

	-speedtest	This option will run Maelstrom in a graphics test
			mode.  It prints the number of milliseconds it
			takes for your graphics display to display a full
			48-frame, 360 degree rotation of your ship.  It is
			for comparative information only.

