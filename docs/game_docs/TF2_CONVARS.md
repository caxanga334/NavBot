# Team Fortress 2 Bot specific ConVars

|ConVar Name|Description|Type|Additional Information|
|:---:|:---:|:---:|:---:|
|sm_navbot_tf_teammates_are_enemies|If enabled, bots will attack teammates.|Boolean|Useful on deathmatch game modes.|
|sm_navbot_tf_force_class|Forces bots to select this class.|String|Must be a valid class name: scout, soldier, pyro, demoman, heavyweapons, engineer, medic, sniper, spies.|
|sm_navbot_tf_force_gamemode|Force a specific game mode.|Integer|Set to a negative value to use the automatic game mode detection.|
|sm_navbot_tf_mvm_collect_currency|If enabled, bots will collect currency in MvM.|Boolean|A third-party plugin is required to patch the game in order to allow RED bots to collect currency.|
|sm_navbot_tf_debug_bot_upgrades|If enabled, prints MvM upgrades debug messages.|Boolean|Cheat protected.|
|sm_navbot_tf_mod_debug|If enabled, prints some TF2 related debug messages.|Boolean|Cheat protected.|

Values for `sm_navbot_tf_force_gamemode`.

* 0: None
* 1: Capture The Flag
* 2: Control Points
* 3: Attack/Defense Control Points
* 4: King of The Hill
* 5: Mann vs Machine
* 6: Payload
* 7: Payload Race
* 8: Player Destruction
* 9: Robot Destruction
* 10: Special Delivery
* 11: Territorial Control
* 12: Arena
* 13: Passtime
* 14: Versus Saxton Hale (Vscript)
* 15: Zombie Infection (Vscript)
* 16: Gun Game
* 17: Deathmatch
* 18: Slender Fortress
* 19: Lizard Of Oz's FFA Deathmatch (Vscript)
* 20: Tug of War

## Notes For TF2 Game Modes

`None` is deathmatch with class behaviors. (IE: medics will heal players, engineers will build.)    
`Deathmatch` is deathmatch without class behaviors. Engineers, medics, snipers and spies won't use their class behavior.    
The following game modes don't have behavior yet: Slender Fortress.    