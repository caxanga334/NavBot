# Nav Volumes

Nav Volumes controls a Nav Area blocked status.

To edit volumes, set the ConVar `sm_nav_volume_edit` to **1**.

## Creating a New Volume

The command `sm_nav_volume_create` will create a new nav volume at your current position.

## Selecting a Volume

In order to make changes to a volume, you need to select it first.

Use the command `sm_nav_volume_select` while aiming at a volume to select it.

Alternatively you can select a specific volume by ID with `sm_nav_volume_select <ID>`.

## Clearing Selection

Use `sm_nav_volume_select clear` or `sm_nav_volume_deselect` to clear the selected volume.

## Deleting a Volume

Use the command `sm_nav_volume_delete` to delete the selected volume.

## Show Areas in Control

To view which nav areas are being controlled by the selected volume, use the command `sm_nav_volume_draw_areas`.

## Editing The Origin

To move the selected volume to a new position, use the command `sm_nav_volume_set_origin`.

The volume will be moved to your current position.

## Editing Bounds

To change the volume size, use the command `sm_nav_volume_set_bounds <mins x> <mins y> <mins z> <maxs x> <maxs y> <maxs z>`.

## Assigning a Team

Volumes can be assigned to a specific team with `sm_nav_volume_set_team <team Index>`.

Use `-2` to make the volume neutral.

## Volume Toggle Condition

The volume's toggle condition can be updated using `sm_nav_volume_set_toggle_condition`.

## Visualize The Target Entity

The command `sm_nav_volume_show_target_entity` will show where the target entity is for ten seconds.

## Conflicts

Nav Areas can only be controlled by one volume. Use the command `sm_nav_volume_check_for_conflicts` to check for conflicts.
