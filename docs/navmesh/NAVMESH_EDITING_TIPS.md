# `sm_nav_slope_limit` and Stairs

On some stairs that level designers did not add an invisible slope to smooth player movement, the navigation mesh generation sometimes partially skips the stairs or doesn't fully connect it.

Using `sm_nav_slope_limit 0.48` generally improves the mesh generation on these stairs.

This hint is not exclusive to NavBot and helps a lot when making nav meshes for Garry's mod since a lot of level designers never heard of stair clipping.

# Finding Entities Indexes

With `sv_cheats 1` and `developer 1`, you can use `ent_text` command to get the entity index.

This will show debug information about the entity you're aiming at. You can also pass a targetname or classname as the first parameter.

# Disconnect Multiple Drop Down Areas

You can disconnect multiple drop down areas with the `sm_nav_disconnect_dropdown_areas <drop down height limit>` command.

Add the areas you want to disconnect to the selected set, then run the command. Any nav areas with a height difference between them larger than the given limit will be disconnected.

# Adjust Area Corner To Player's Z

The command `sm_nav_corner_place_at_feet (adjust nearby)` can be used to adjust the vertical position of the currently select area corner.    
The command has one optional boolean parameter that determines if the vertical position of nearby areas should also be adjusted.

# Testing Path Finding Without Bots

The command `sm_navbot_tool_build_path <preset>` can be used to build the shortest path to a given area.    
Use `sm_nav_mark` to mark the path **goal** area. The nearest area to you is used as the path start area.    
The command has one argument, the name of the path cost preset you want to use.

# Vertical Jumps

Nothing special is needed for vertical jumps. A standard connection is all that is needed for vertical jumps.    
During path generation, the height difference between two nav areas is measured and a jump or double jump instruction is created if needed.    
If an area is too high to reach by jumping, bots will look for another path.    
The **JUMP** attribute set by the `sm_nav_jump` is not used. This is a legacy nav attribute used by CSBots.    

# Horizontal Jumps

Nothing special is needed, just create a standard connection between two areas, a jump over gap instruction is created based on the distance between the two nav areas.

# Railings and Jumps

It's common for the generator to generate a one-way connection near railings. Bots frequently gets stuck because they don't know they need to jump.    
To fix this, disconnect the areas first, then create a new area between the floor and ledge. Jump onto the railing, mark the new area with `sm_nav_mark` and them run this command `sm_nav_corner_place_at_feet false` to move the newly created area to the railing's height. Then create a two-way connection from the floor to the new area and a one-way to the ground below to create a drop down.    
The bot's path generation will now understand that it needs to jump before dropping.