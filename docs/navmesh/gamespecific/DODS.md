# Day of Defeat: Source Nav Area Attributes

Documention of Day of Defeat: Source specific nav mesh features.

## DoD Attributes

To assign DoD specific attributes, use the `sm_dod_nav_set_attribute` to toggle dod attributes on nav areas.    
You can remove all DoD attributes with `sm_dod_nav_wipe_attributes`.    
Use `sm_dod_nav_select_areas_with_attributes` to add areas with specific DoD attributes to the selected set.    

Attribute list:    

- noallies : Nav area is always blocked for bots on the allies team.
- noaxis : Nav area is always blocked for bots on the axis team.
- bombs_to_open : This area is obstructed by an obstacle that can be cleared with a bomb.
- requires_prone : Tell bots they need to prone to traverse this nav area.

## Bomb Obstructions

If a passage is blocked by an obstruction that can be cleared by planting a bomb, use the `bombs_to_open` attribute to signal bots they need a bomb to clear it.    