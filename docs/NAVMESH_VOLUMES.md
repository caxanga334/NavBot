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

## Block Condition Check Type

Block condition determines when the volume will be considered blocked.

The condition check type is set by the command `sm_nav_volume_set_check_condition_type <type ID>`.

Use the command `sm_nav_volume_list_condition_types` to get a list of all IDs.

Use the command `sm_nav_volume_set_check_condition_inverted` to enable/disable inverted logic mode.

## Target Entity

Some block conditions requires a target entity.

To set the target entity, use the command `sm_nav_volume_set_check_target_entity <index or targetname>`.

Hint: Built-in dev commands like `ent_text` can be used to retrieve both index and targetname.

## Block Condition Float Data

Some block conditions requires additiona data, to set it, use this command: `sm_nav_volume_set_check_float_data <value>`.

## Visualize The Target Entity

The command `sm_nav_volume_show_target_entity` will show where the target entity is for ten seconds.

## Conflicts

Nav Areas can only be controlled by one volume. Use the command `sm_nav_volume_check_for_conflicts` to check for conflicts.
