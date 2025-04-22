# Basic Usage

The bot's difficulty can be selected with `sm_navbot_skill_level`.  
The bot comes with four difficulty presets by default: Easy, Normal, Hard and Expert.  
The default difficulty used is **Easy**.  
For more information, see [Bot Difficulty Configuration].

## Adding Bots

You can add a single bots with the `sm_navbot_add` command. The bot stays in-game until a level change.

If you wants bots to join automatically, you can enabled the bot quota system.  
First you need to select which mode you want: Normal or Fill.  
On **normal** mode, an N number of bots is added to the game.  
On **fill** mode, NavBot will keep an N number of players in-game, adding and removing bots as human players joins and leaves the game.  
The mode is set via the `sm_navbot_quota_mode` ConVar.  
Either `sm_navbot_quota_mode normal` or `sm_navbot_quota_mode fill`.
The target number of bots is set by `sm_navbot_quota_quantity`. Set this ConVar to **0** to disabled the bot quota system.


<!-- Links -->
[Bot Difficulty Configuration]: BOT_DIFFICULTY_PROFILES.md
