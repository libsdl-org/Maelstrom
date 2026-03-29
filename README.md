# Maelstrom 4.0

You pilot your ship through the dreaded "Maelstrom" asteroid belt -- suddenly your best friend thrusts towards you and fires, directly at your cockpit. You raise your shields just in time, and the battle is joined.

The deadliest stretch of space known to mankind has just gotten deadlier. Everywhere massive asteroids jostle for a chance to crush your ship, and deadly shinobi fighter patrols pursue you across the asteroid belt. But the deadliest of them all is your sister ship, assigned to you on patrol. The pilot, trained by your own Navy, battle hardened by months in the Maelstrom, is equipped with a twin of your own ship and intimate knowledge of your tactics.

The lovely Stratocaster R&R facility never sounded so good, but as you fire full thrusters to dodge the latest barrage you begin to think you'll never get home...

---
Maelstrom is an open source port of the original shareware game for the Macintosh by Ambrosia Software. It is a fast-action, asteroids-like game, with classic pixel graphics and original sounds.

In addition to being a faithful reproduction of the original game, this version adds game controller support, mobile touch controls, and both cooperative and competitive multiplayer action.

Maelstrom is available for free on:
- Steam: https://store.steampowered.com/app/4239950/Maelstrom
- Google Play: 
- Apple Store: 
- Web: https://www.libsdl.org/projects/Maelstrom/play

You can download the latest release and source code from [GitHub](https://github.com/libsdl-org/Maelstrom)

## New Features

### Kid Mode

Clicking the "kid mode" icon next to the play button will enable an easier play mode which will add the retro-thruster prize and automatically shield you as long as shields are available - but beware the Shenobi ships! Their shots will not trigger the automatic defensive systems.

Achievements and high scores are not available while in this mode.

### Multiplayer

You can host or join a multiplayer game from the main menu. Up to three players can play at once, and they can be any combination of other players on the local network, connected controllers, and friends invited via Steam Remote Play.

### Steam Achievements

If you play the game on Steam, there are lots of fun achievements to unlock, such as "Alien Abduction" and "Balls of Steel".

### Steam Game Recording

If you play the game on Steam and enable game recording, the timeline will automatically be populated with exciting events such as aliens, nova explosions, and more!

### Replays

You can click on the high score entries to watch that game again. This is especially fun with the Steam game recording feature, as you can see the events on the timeline and re-share the game video with your friends.

### Easter Eggs

The classic easter eggs from the original game are all there, and it's up to you to find them...

### Addons

The art and sounds for the game are in the Data directory and can be freely modified for your own use.

If you have access to the original sound and sprite packs for Maelstrom, you can build Maelstrom from source and use the included tool `macres` to unpack them into the Data directory to change the art and sounds for the game:
```
macres --export '%Maelstrom Sprites' Data
macres --export '%Maelstrom Sounds' Data
```

If you play network multiplayer, all players must have the same set of sprites, otherwise the games will get out of sync.

---

Enjoy!

Sam Lantinga (slouken@libsdl.org)
