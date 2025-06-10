# Simple Ladders

Simple ladders are ladders generally made of world geometry. They are the ladder type used in game such as Counter-Strike: Source and Day of Defeat: Source.

## Building Simple Ladders

Aim at the ladder, the crosshair will change colors, if it does not then the ladder is not getting recognized as a ladder.      
Run the command `sm_nav_build_ladder`, then connect and/or disconnect nav areas if needed.

# Useable Ladders

NavBot added support for Half-Life 2 style useable ladders. They are also present in HL2 based games such as Half-Life 2: Deathmatch and Garry's mod.    
Hint: Use `ent_text func_useableladder` to make the ladder entity visible.    

## Building Useable Ladders

Go near the ladder and run the following command: `ent_text func_useableladder`
You will most likely need to fix the ladder's direction. Look at the ladder and run this command: `sm_nav_useable_ladder_set_dir`    
Then check if there are connected areas to the bottom and top of the ladder.

# Additional Information

NavBot's nav mesh ladders supports multiple connections at different heights while the original Valve implementation doesn't.