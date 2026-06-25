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
