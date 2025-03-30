# Nav Elevators

Nav Elevators are an expanded elevator support added to NavBot's navigation mesh.

Set the ConVar `sm_nav_elevator_edit` to `1` to enable elevator edit mode.

The following elevator entities are supported:

* func_door
* func_movelinear
* func_tracktrain

Left 4 Dead/2's func_elevator is recognized but not implemented.

## Creating a New Elevator

Use the command `sm_nav_elevator_create <elevator entity index>` to create a new elevator for the given elevator entity.

It's recommended to use this command when the elevator is at the map spawn position. (The position the elevator is when the map is loaded.)

## Selecting Elevators

Edit commands needs the elevator to be selected.

Use `sm_nav_elevator_select_nearest` to select the nearest elevator.

Use `sm_nav_elevator_select_id <id>` to select an elevator via ID number.

Use `sm_nav_elevator_clear_selection` to clear the selected elevator.

## Deleting Elevators

Use `sm_nav_elevator_delete` to delete the currently selected elevator.

## Updating Elevator Origin

Use `sm_nav_elevator_update_position` to update the elevator map origin if you created it while the elevator was not on the map spawn position.

## Changing Floor Detection Distance

For elevators that doesn't have a toggle state (IE: func_tracktrain), the current floor is detected by checking the distance of the elevator to each floor.

The default distance to consider an elevator to be on a floor is 48 hammer units. This distance can be set with the `sm_nav_elevator_set_floor_detection_distance <distance>` command.

## Team Restriction

If the elevator can only be used by one team, use `sm_nav_elevator_set_team <team index>` to assign it to a team.

Use `-2` to remove team restrictions.

## Automatic Elevators

If the elevator is automatic (activated when you enter it), use `sm_nav_elevator_toggle_type` to toggle between manual/automatic type.

## Adding Floors

To create a new floor, create a nav area inside the elevator, then mark it (with `sm_nav_mark`) and run the command `sm_nav_elevator_add_floor` to create a new floor.

## Assigning a Call Button

The call button is the button the bot needs to press in order to call the elevator to the floor the bot is currently at.

To assign a button, use `sm_nav_elevator_set_floor_call_button <entity index>`.

## Assigning a Use Button

The use button is the button the bot needs to press in order to active the elevator.

Each floor should have one assigned unless it's an exit only floor (the elevator stops and then automatically returns).

To assign a button, use `sm_nav_elevator_set_floor_use_button <entity index>`.

## Toggle Useable/Shootable Button

Use `sm_nav_elevator_toggle_floor_shootable_button` to toggle between shootable and useable buttons for this floor.

Most games uses useable buttons, some games like TF2 may use shootable buttons instead. 

Buttons are useable by default.

## Setting The Wait Position

The wait position is the position the bot will move and wait for the elevator. It is also used as an exit position when exiting the elevator.

Each floor should have one.

To set the wait position, use `sm_nav_elevator_set_floor_wait_position`. Your current position will be used.

## Setting The Floor Position

The floor position is the position used to detect the elevator. It's only required for elevators that doesn't have a toggle state (see elevator types).

To set the floor detection position, use `sm_nav_elevator_set_floor_position`. Your current position will be used.

## Setting The Floor Toggle State

Some elevator types have toggle states (func_door, func_movelinear).

For these elevators, the floor detection checks the elevator's current toggle state.

To assign a toggle state value to a floor, use `sm_nav_elevator_set_floor_toggle_state <state>`.

There are only two valid states: 0 and 1.

A `Elevator is here!` text should appear on the floor the elevator is. If it does not appear then the current toggle state doesn't match the floor toggle state.

## Deleting Floors

You can delete a marked floor with `sm_nav_elevator_delete_marked_floor`.

You can delete all floors with `sm_nav_elevator_delete_all_floors`.

# Elevator Types

The elevator type is show on the screen for the currently selected elevator.

| Value | Meaning |
|:---:|:---:|
| 0 | Undefined (ERROR) |
| 1 | Auto Trigger (Activated automatically) |
| 2 | func_door Elevator |
| 3 | func_movelinear Elevator |
| 4 | func_tracktrain Elevator |
| 5 | func_elevator Elevator |