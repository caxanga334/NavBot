# Zombie Panic! Source Nav Mesh

## ZPS Attributes

* `nozombies`: Bots on the zombie team cannot traverse this area.
* `nohumans`: Bots on the human team cannot traverse this area.
* `transient_humans`: Enables the phys prop transient check. If the area is obstructed by phys props, the area is blocked for humans only.
* `transient_physprops`: Same as above but applies to both humans and zombie teams.

## Commands

`sm_zps_nav_wipe_attributes`: Removes all ZPS attributes from selected areas.    
`sm_zps_nav_set_attribute`: Adds ZPS attributes to selected areas.
`sm_zps_nav_select_areas_with_attributes`: Adds areas that contains the given ZPS attributes to the selected set.

# General Tips

## Test Mode/Solo Play

ZPS requires 2 or more players, to edit the nav mesh solo, you can enable test mode (`sv_testmode 1`) or solo play (`sv_zps_solo 1`).    
Test mode allows you to freely switch between teams.    

## Dynamic Maps

Some maps change layouts based on player count, to test this, disable solo play and test mode, enable the test bots (`sv_zps_bot_enable 1`) and then add bots using the `bot_zps_add_*` commands.    
Add/remove bots, restart the round and see how the layout changes.    

## Restarting the Round

Enable and disable test mode to force a round restart and reset the map, you need to wait a few seconds before enabling test mode again otherwise the round doesn't restart.    

## Team Switch

* `choose1`: Joins survivors (default key bind: F1)
* `choose2`: Joins zombies (default key bind: F2)
* `choose3`: Joins spectator (default key bind: F3)
* `choose4`: Goes back to the ready room (default key bind: F4)

