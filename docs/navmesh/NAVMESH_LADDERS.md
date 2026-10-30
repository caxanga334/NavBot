# Simple Ladders

Simple ladders are ladders generally made of world geometry. They are the ladder type used in game such as Counter-Strike: Source and Day of Defeat: Source.

## Building Simple Ladders

Aim at the ladder, the crosshair will change colors, if it does not then the ladder is not getting recognized as a ladder.      
Run the command `sm_nav_build_ladder`, then connect and/or disconnect nav areas if needed.

# Useable Ladders

NavBot added support for Half-Life 2 style useable ladders. They are also present in HL2 based games such as Half-Life 2: Deathmatch and Garry's mod.    
Hint: Use `ent_text func_useableladder` to make the ladder entity visible.    

## Building Useable Ladders

Go near the ladder and run the following command: `sm_nav_build_useable_ladder`
You will most likely need to fix the ladder's direction. Look at the ladder and run this command: `sm_nav_useable_ladder_set_dir`    
Then check if there are connected areas to the bottom and top of the ladder.

# Team Specific Ladders

Ladders can now be made team specific with the `sm_nav_ladder_set_blocked` console command.    
First mark a ladder with the `sm_nav_mark` command.    
Usage: `sm_nav_ladder_set_blocked <team index> <yes/no>`    
Use **-2** to update for all teams.    
If the second parameter is *yes*, the ladder is marked as blocked for the given team.    
If *no*, it's marked as unblocked.    
The blocked status is static and saved to the nav mesh file.    
Examples:    
- `sm_nav_ladder_set_blocked -2 no` - Makes the marked ladder **not blocked** for all teams.
- `sm_nav_ladder_set_blocked 3 yes` - Makes the marked ladder **blocked** for team 3.

## Checking Team Restriction Status

To check a ladder's team restrictions, mark a ladder and run the `sm_nav_ladder_query_blocked` command.    

# Additional Information

NavBot's nav mesh ladders supports multiple connections at different heights while the original Valve implementation doesn't.