<!-- Documentation for basic navigation mesh editing specific to NavBot customized Nav Mesh -->

# Navigation Mesh Editing Basics

Since NavBot uses a modified Valve's Navigation Mesh, basic editing is pretty similar.

In order to edit the navigaiton mesh, you must load NavBot on a Listen Server since the edit commands are server side and the debug overlay is used to draw the nav mesh.

Begin by loading the map you want to make a nav mesh for.

NavBot uses the same commands as valve's navigation mesh but with a **sm_** prefix added. 

## Enabling Edit mode

Enable cheats (`sv_cheats 1`) and then to enable edit mode: `sm_nav_edit 1` into the console.

## Mesh Generation

I do not recommend using `sm_nav_generate`. Instead use incremental generation.

First, run the auto walkable seed by using the following command: `sm_nav_seed_walkable_spots`

This command will automatically add walkable seeds in locations that are potentially walkable on the map.

Then walk around the map with noclip and add more walkable seeds (command: `sm_nav_mark_walkable`) to any areas that need it, it may take a few incremental generations to fully mesh the entire map.

To generate the mesh, run the following command: `sm_nav_generate_incremental`.

## Clean Overlapping Areas Post Incremental Generation

Sometimes the generator creates overlapping areas post an incremental generation. These areas can be cleaned up with the `sm_nav_delete_overlapping_from_selected_set` command.

After an incremental generation, if you see any overlapping areas, keep the currently selected areas and run the command, the overlapping areas should get deleted, then check if there are any connections that needs to be created.

## Saving

Use `sm_nav_save` to perform a quick save. 

Once you're done editing, disable quick save (`sm_nav_quicksave 0`) and then do a full nav analyze with `sm_nav_analyze`.