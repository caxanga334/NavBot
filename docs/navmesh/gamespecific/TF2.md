# Team Fortress 2 Nav Mesh Specifics

Most game modes doesn't need any specific flags/attributes to work.

Currently, TF2's nav areas have three attribute groups: TFAttributes, TFPathAttributes, MvMAttributes

## Path Attributes (TFPathAttributes)

These attributes are used by the bot path finding.

They can be set via the `sm_tf_nav_toggle_path_attrib` console command.

Available attributes:

- noredteam : Bots from RED team can't path through this area.
- nobluteam : Bots from BLU team can't path through this area.
- noflagcarriers : Bots carrying the flag/intelligence can't path through this area.
- flagcarriersavoid : This area has increased path cost for bots carrying the flag/intelligence.
- spawnroom : This area is inside a spawn room. See below for more details.

### Spawn Rooms

The `spawnroom` makes the area scans for the nearest `func_respawnroom` entity and assign itself to the entity's team.

This tell bots from the opposite team that they can't navigate these areas.

Engineers will also known they can't build on these areas.

The command `sm_tf_nav_auto_set_spawnrooms` can be used to automatically add the `spawnroom` attribute to all areas that are inside the boundaries of a `func_respawnroom` entity.

## TF Attributes

Legacy, currently unused. Replaced by waypoint hints.

## MvM Attributes

Available attributes:

- frontlines : Area where bots will wait before the wave starts. Bots will only toggle ready if their current area has this attribute.
- upgradestation : Area where bots can go to buy upgrades, make sure the nav area is smaller than the upgrade station entity bounds.

All MvM maps needs to have at least one `upgradestation` and `frontlines` areas.

They can be set via the `sm_tf_nav_toggle_mvm_attrib` command.

# Team Fortress 2 Waypoint Specifics

Waypoints in tf2 are used to provide bot with hints such as sentry and sniper spots, see below for more details.

## Waypoint Hints

TF2 waypoints can be assigned a *hint*.

These are:

0. None
1. Guard : Unused, was going to be used for defend positions.
2. Sniper : Spots where snipers will snipe from.
3. Sentry Gun : Engineers will build a sentry gun here.
4. Dispenser : Engineers will build a dispenser here.
5. Teleporter Entrance : Engineers will build a teleporter entrance here.
6. Teleporter Exit : Engineers will build a teleporter exit here.

These waypoints are optional, snipers and engineers will function without them. They are used to make bots snipe/build in less random places.

Dispenser and teleporter waypoints can be connected to a sentry waypoint, engineers will priorize using connected waypoints first.

Sniper and engineer waypoints should have at least one angle set, see [Waypoint Basics] for more details.

To assign a hint, select the waypoint and run this command: `sm_tf_nav_waypoint_set_hint <hint number>`.

Use `sm_tf_nav_waypoint_list_all_hints` to get a list of hints and their ID.

## Control Points

Waypoints can be assgined to a control point index with the `sm_tf_nav_waypoint_set_control_point <control point index>` command.

When a control point is assigned to a waypoint, bots will only use the waypoint if their team can either attack or defend the assigned control point.

To clear the assigned control point, use `sm_tf_nav_waypoint_set_control_point -1`.

[Waypoint Basics]: WAYPOINT_BASICS.md
