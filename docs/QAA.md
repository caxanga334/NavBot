# Questions & Answers

## Why are the bots not moving?

The primary cause for bots not moving is a lack of a navigation mesh for the current map.    
Run the following command: `sm_navbot_info`    
Check the console for the output, if it says that the Nav Mesh is `NOT Loaded` then the current map lacks one.    
The secondary cause is lack of support for the current mod or game mode.
The nav mesh generator isn't perfect and may some times leaves areas unconnected.
If you're on a listen server, run this command `sm_navbot_debug TASKS PATH MOVEMENT` to enable debugging.    
