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

# Advanced Place On Ground

The `sm_nav_corner_place_on_ground_custom <offset number or "origin"> [raise adjacent]` offers a bit more control over the default place on ground command.    
It takes two parameters, the first one can either be a height offset number or a `"origin"` string.    
If `"origin"` is passed, the trace start position will use the listen server host height. If a number is passed, it will sum the given offset to the corner's height.    
The second parameter is optional and is default true. If true, adjancent areas will also be raised. Pass false to only raised the marked area.    

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

# Transient Attribute

The **TRANSIENT** attribute can be applied to areas with the `sm_nav_transient` command.    
This attribute tells the nav mesh that this nav area should be periodically checked for obstruction.    
If an obstruction is found, the area is marked as blocked and bots won't path through it.    
A ground check is also performed and areas floating in the air will also be marked as blocked.    
This attribute blocks the area for all teams. If you need a more advanced control of an area blocked/unblocked status, see [Nav Mesh Volumes].    

# Checking Blocked Status

The command `sm_nav_draw_blocked_status <team>` can be used to check the blocked status of nav areas from the selected set, marked area or the area currently under your cursor.    
Blocked areas will be filled in orange and unblocked areas will be filled in blue.    
The `<team>` parameter accepts the following values:    
* ANY: Blocked for any team.
* MY: Blocked for your current team.
* Team Number: Blocked for the given team number.

# Overriding an Entity Walkable Status

Some entities are considered walkable by the nav mesh, so they don't collide with raycast traces done by the nav mesh. For example, doors are considered walkable.
Most of the time this is fine however sometimes this becomes a problem when nav areas are generated on places that are solid for players.    
Another case is on some maps there are temporary bridges that are non-solid for the nav mesh and manually creating the areas on these bridges is hard.    
NavBot offers some control over which entities are solid for nav mesh generation and editing:

* `sm_nav_solid_props`: This ConVar makes props (both physics and dynamic) solid. (Note: This ConVar exists on Valve's implementation, NavBot fixed it not doing anything).
* `sm_nav_solid_func_brush`: NavBot exclusive convar to force func_brush entities to be always solid.
* `sm_nav_trace_make_entity_solid <optional: ent index>`: Console command to add the entity you're looking at to the list of forced solid entities. You can also pass an entity index as the first parameter.

**Important Notes**:    
Use `sm_nav_trace_clear_solid_entity_list` to clear the forced solid entity list. This list is also automatically cleared when a new round starts.    
**func_brush** entities must be in their solid state otherwise the engine sees it as non-solid and traces won't collide with them. You can use the `ent_fire` command to change their solid state.    
`sm_nav_solid_props` includes prop_physics entities, you may want to generate the initial mesh with it turned off and enable when needed.    

# Generating Mesh On Dynamic Props

Some maps may have large prop_dynamic entities and these aren't solid to nav mesh generation by default.    
To make these props solid for nav generation, you need to set `sm_nav_solid_props` to 1 and change the nav mesh generation trace mask to a mask that collides with props.    
Use the command `sm_nav_change_generation_trace_mask ` to change the nav mesh generation mask. Using `MASK_PLAYERSOLID` is recommended for props.    

# Bypassing Auto Blockers

In some games, the nav mesh may automatically mark areas as blocked.    
These areas will have a **AUTOBLOCKER** text when aiming at them.    
If the auto blocker is incorrectly marking an area as blocked, they can be bypassed by adding the **NO AUTO BLOCKERS** attribute to areas using the `sm_nav_no_auto_blockers` console command.    

# Drawing Incoming One-ways

The drawing of incoming one-way connections to nav areas can be enabled using the `sm_nav_show_incoming` ConVar.    

# Selecting Areas Touching an Entity

The command `sm_nav_select_areas_touching_entity <ent index>` will add all areas that overlaps the given entity AABB and also collides against the entity to the selected set.    

# Underwater Navigation 

If the height difference between two nav areas is greater than the bot's step height, the bot will send move up/move down buttons to change height.    

# Water Exits

It's recommended to create a dedicated water exit nav area on the water's surface.    
`Underwater nav area -> nav area on water surface -> nav area outside water.`
To create a nav area on the surface, create a nav area on the ground, mark with `sm_nav_mark` (you can pass the created area ID as the first parameter) and run `sm_nav_corner_place_on_water_surface`.    

# Force Ladders

If the nav mesh fails to recognize a surface as climbable, you can set the `sm_nav_force_climbable` to **1** to force every surface to be climbable and create the ladder manually.    

<!-- LINKS -->
[Nav Mesh Volumes]: NAVMESH_VOLUMES.md
