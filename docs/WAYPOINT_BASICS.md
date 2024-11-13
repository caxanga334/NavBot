# Base Waypoint Editing Tutorial

## What are waypoints?

Unlike other bots such as [RCBot2], NavBot does not use waypoints for pathfinding.

Their purpose is to provide hints that require precise positioning such as sniper spots.

Waypoints are saved on the navmesh file.

# Waypoint Editor Usage

Enable nav mesh editing, then to enable waypoint editing, the set `sm_nav_waypoint_edit` ConVar to **1**.

## Command List

| --- | --- | --- |
| Command | Arguments | Description |
| sm_nav_waypoint_add | None | Creates a new waypoint at your current position. |
| sm_nav_waypoint_delete | None | Deletes the selected waypoint or nearest if none is selected. |
| sm_nav_waypoint_mark_crosshair | None | Selects the nearest waypoint to your crosshair. |
| sm_nav_waypoint_mark_nearest | None | Selects the nearest waypoint to your current position. |
| sm_nav_waypoint_mark_id | <Waypoint ID> | Selects the waypoint with the specific ID. |
| sm_nav_waypoint_unmark | None | Clears the selected waypoint. |
| sm_nav_waypoint_connect | <From ID> <To ID> | Creates a one-way connection between two waypoints. |
| sm_nav_waypoint_disconnect | <From ID> <To ID> | Removes a one-way connection between two waypoints. |
| sm_nav_waypoint_info | None | Shows information of the selected waypoint. |
| sm_nav_waypoint_set_radius | <Radius> | Sets the waypoint radius. |
| sm_nav_waypoint_add_angle | None | Adds a new angle to the selected waypoint. |
| sm_nav_waypoint_update_angle | <Angle Index> | Updates the angle value of the given angle index. |
| sm_nav_waypoint_clear_angles | None | Removes all angles from the selected waypoint. |
| sm_nav_waypoint_set_team | <Team Index> | Sets the team index that owns this waypoint. |

# Aditional Information

## What are connections for?

Most NavBot waypoints doesn't use connections.

They have the purpose of linking two waypoints together. Some waypoints can be linked to provide additional information to the bot. 
This depends on the waypoint implementation for the current mod.

## Team Indexes

### Common Values

The `Any Team` is the default team a waypoint is assigned to and applies to all mods.

Most mods also doesn't change the unassigned and spectator teams.

| --- | --- |
| Index | Meaning |
| -2 | Any Team (Default) |
| 0 | Unassigned Team |
| 1 | Spectator Team | 

### Team Fortress 2

| --- | --- |
| Index | Meaning |
| 2 | RED Team |
| 3 | BLU Team |

## Angles

If you worked with [RCBot2] waypoints before, this is a similar feature to the waypoint *yaw*.

However instead of only storing a single *yaw*, NavBot's waypoints can have up to 4 full angles (pitch, yaw and roll).

These angles are used when bots have to aim/look at a specific angles, such as sniper spots. The specific usage of the angles depends on the implementation for that specific waypoint and mod.

## Radius

Radius is used to randomize the position the bot will go to when using a waypoint.

The default value is 0.


<!-- LINKS -->
[RCBot2]: https://github.com/rcbotCheeseh/rcbot2