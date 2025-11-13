# Prerequisites

Prerequisites is a volume that inform bots they need to complete a task ir order to move through the nav areas inside the prerequisite bounds.

NavBot version of [func_nav_prerequisite](https://developer.valvesoftware.com/wiki/Func_nav_prerequisite).

To enable editing, set `sm_nav_prerequisite_edit` to 1.

# Editing Commands

List of console commands used for editing prerequisites.

These commands are server commands, they should be used only by a listen servern host.

## sm_nav_prereq_create

Creates a new prerequisite.

## sm_nav_prereq_mark

If a prerequisite is not selected, selects the nearest prerequisite.

Alternatively, you can pass the prerequisite ID as the first parameter to select it.

Clears the selection if a prerequisite is selected.

## sm_nav_prereq_delete

Deletes the currently selected prerequisite.

## sm_nav_prereq_draw_areas

Draws the nav areas inside the currently selected prerequisite bounds.

## sm_nav_prereq_set_origin

Updates the currently selected prerequisite origin to your current position.

## sm_nav_prereq_set_bounds

Updates the currently selected prerequisite bounds. Takes 6 arguments.

`sm_nav_prereq_set_bounds <mins x> <mins y> <mins z> <maxs x> <maxs y> <maxs z>`

## sm_nav_prereq_set_team

Assigns the prerequisite to a team. First argument is the team index. A team index of -2 can be used to set it for all teams.

## sm_nav_prereq_set_task

Assigns the task the bot needs to complete. The first argument is the task ID.

## sm_nav_prereq_list_available_tasks

List all available tasks and their IDs.

## sm_nav_prereq_set_goal_position

Sets the task goal position vector.

## sm_nav_prereq_set_goal_entity

Sets the task goal entity.

## sm_nav_prereq_set_goal_data

Sets the task goal data.

## sm_nav_prereq_set_toggle_condition

Sets the prerequisite toggle condition used to determine if the prerequisite is enabled or disabled.

# Task Types

## TASK_WAIT

The bot will wait for N seconds where N is defined by the prerequisite task goal data.

## TASK_MOVE_TO_POS

The bot will move to the prerequisite task goal position.

## TASK_DESTROY_ENT

The bot will move to and attack the prerequisite task goal entity.

## TASK_USE_ENT

The bot will move to and press their use button on the task goal entity.

# Using Prerequisites

Go to where you want to create a prerequisite and use the `sm_nav_prereq_create` command. A prerequisite will be created at your current position.    
Bots will complete a prerequisite when they enter a nav area that has a prerequisite assigned to it. See *Assigning Nav Areas*.    
Prerequisites can be team specific, use `sm_nav_prereq_set_team <team index>` to restrict which team will use it. Use a value of `-2` for **any** team.

## Assigning Nav Areas

Nav areas whose center is inside the prequisite bounds are assigned to it.    
Use the `sm_nav_prereq_set_bounds <mins x> <mins y> <mins z> <maxs x> <maxs y> <maxs z>` command to update the prerequisite size and `sm_nav_prereq_set_origin` to update the prerequisite position.
To visualize which areas are currently assigned to a prerequisite, run the `sm_nav_prereq_draw_areas` command.    

## Setting The Task

The `sm_nav_prereq_set_task <task ID Number>` sets the task the bot will complete. Use `sm_nav_prereq_list_available_tasks` to get a list of tasks and their ID number.    
Tasks have a goal position, entity and data. **WAIT** tasks uses the task data, the data is the number of seconds a bot should wait.    
Entity is used by **USE_ENT** and **DESTROY_ENT** and position is used by **MOVE_TO_POS**.

## Setting The Toggle Condition

The toggle condition is used to determine if the prerequisite is enabled/disabled.    
The toggle condition is set via the `sm_nav_prereq_set_toggle_condition` command.    
Run the `sm_nav_prereq_set_toggle_condition` without any arguments to print usage instructions.    

