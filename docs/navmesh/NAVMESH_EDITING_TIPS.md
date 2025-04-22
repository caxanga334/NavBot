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
