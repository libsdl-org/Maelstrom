
Networking has been tested on Linux, SGI and DEC.  It is known to
work between SGI and DEC systems (cross-platform), and between systems
of the same type.

Up to three players can play, and the syntax is as follows:

(On machine 1)
	Maelstrom -player 1 -player 2@machine2 -player 3@machine3

(On machine 2)
	Maelstrom -player 1@machine1 -player 2 -player 3@machine3

(On machine 3)
	Maelstrom -player 1@machine1 -player 2@machine2 -player 3


You can also use a network address server that handles the difficulties
of getting all the players and addresses right.  The address server is
installed as 'Maelstrom-netd', and if run on say "servhost", the syntax
for a three player game would be:

(On machine A)
	Maelstrom -player 1 -server 3@servhost

(On machine B)
	Maelstrom -player 2 -server 3@servhost

(On machine C)
	Maelstrom -player 3 -server 3@servhost

The address server will let players know if they have conflicting
player numbers, and will set up the proper addresses for all players.
It can field multiple games, and can be a "game hub" where a bunch
of players can get together for romping network games. :)

