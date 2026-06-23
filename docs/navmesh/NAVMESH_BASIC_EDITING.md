<!-- Documentation for basic navigation mesh editing specific to NavBot customized Nav Mesh -->

# Navigation Mesh Editing Basics

Since NavBot uses a modified Valve's Navigation Mesh, basic editing is pretty similar.    
In order to edit the navigaiton mesh, you must load NavBot on a Listen Server since the edit commands are server side and the debug overlay is used to draw the nav mesh.    
Begin by loading the map you want to make a nav mesh for.    
NavBot uses the same commands as valve's navigation mesh but with a **sm_** prefix added.    

## Enabling Edit mode

Enable cheats (`sv_cheats 1`) and then to enable edit mode: `sm_nav_edit 1` into the console.

## Mesh Generation

When generating a nav mesh for the first time, use `sm_nav_generate`.    
If you need to generate the nav mesh again partially, use `sm_nav_mark_walkable` to add a walkable seed to the place that you want to generate a mesh for and run the `sm_nav_generate_incremental` command.    

## Clean Overlapping Areas Post Incremental Generation

Sometimes the generator creates overlapping areas post an incremental generation. These areas can be cleaned up with the `sm_nav_delete_overlapping_from_selected_set` command.    
After an incremental generation, if you see any overlapping areas, keep the currently selected areas and run the command, the overlapping areas should get deleted, then check if there are any connections that needs to be created.    

## Saving

Use `sm_nav_save` to perform a quick save.    
Once you're done editing, disable quick save (`sm_nav_quicksave 0`) and then do a full nav analyze with `sm_nav_analyze`.    
