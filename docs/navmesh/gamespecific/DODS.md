# Day of Defeat: Source Nav Area Attributes

Documention of Day of Defeat: Source specific nav mesh features.

## DoD Attributes

To assign DoD specific attributes, use the `sm_dod_nav_set_attribute` to toggle dod attributes on nav areas.    
You can remove all DoD attributes with `sm_dod_nav_wipe_attributes`.    
Use `sm_dod_nav_select_areas_with_attributes` to add areas with specific DoD attributes to the selected set.    

Attribute list:    

- noallies : Nav area is always blocked for bots on the allies team.
- noaxis : Nav area is always blocked for bots on the axis team.
- blocked_until_bombed : Nav area is blocked until the nearest bomb target explodes.
- blocked_without_bombs : Nav area is blocked for bots that doesn't have a bomb in their inventory.
- plant_bomb : Tell bots they need to plant a bomb at the nearest bomb target of the nav area.
- requires_prone : Tell bots they need to prone to traverse this nav area.