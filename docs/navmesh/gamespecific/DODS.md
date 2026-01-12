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

## Waypoint DoD Flags

DoD exclusive waypoint flags can be toggled using the `sm_dod_nav_waypoint_toggle_dod_attribs <flag>` command.    
The following flags/attributes are available:

- MGNEST: Bots with machine guns will deploy here.
- PRONE: Bots will go prone when using this waypoint.
- POINT_OWNERS_ONLY: This waypoint is only available to the team that owns the assigned control point.
- POINT_ATTACKERS_ONLY: This waypoints is only available to the team that doesn't own the assigned control point.
- USE_RIFLEGREN: Bots will use rifle grenades here.

## Waypoint Control Points

Waypoints can be assigned to a control point using the `sm_dod_nav_waypoint_set_control_point <index>` command.    
Use `-1` to unassign a control point from a waypoint.    
You can get a list of control points and their indexes with the `sm_dod_navbot_debug_control_point_index` command.    
When a control point is assigned to a waypoint, bots will only be able to use the waypoint if they can either attack or defend the control point.    
If the `POINT_OWNERS_ONLY` flag is set, then only the team that owns the assigned control point is allowed to use the waypoint, this can be used to create a waypoint that should only be used by the defending team.    
The `POINT_ATTACKERS_ONLY` flag is the reverse of `POINT_OWNERS_ONLY`. Only the attacking team will be allowed to use.    
