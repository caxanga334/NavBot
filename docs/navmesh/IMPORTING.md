# Importing an Existing Navigation Mesh File

NavBot supports importing and converting official navigation mesh files into NavBot's format.  
To import a nav mesh, run this command: `sm_nav_import`.    
Note: Importing can only be done when no nav mesh is present.    

After importing, run `sm_nav_analyze` with quick save disabled.

## Supported Games

NavBot can import navigation mesh files from the following games:    

* Team Fortress 2 (Version 16, Subversion 2)
* Unmodified Source SDK 2013 nav mesh (Version 16, Subversion 0)

## Automatic Import

Setting the ConVar `sm_nav_auto_import` to 1 will enable automatic import.    
When a map with no nav mesh is loaded, it will run the `sm_nav_import` command automatically.    
The auto import feature works on dedicated servers.    

# Importing RCBot2 Waypoints

NavBot can parse and convert RCBot2 waypoints into NavBot's format.    
Because NavBot uses the nav mesh for navigation, one must exists before importing RCBot2 waypoints.    
NavBot will only import waypoints that are useful for NavBot (example: sniper waypoints).    
To import RCBot2 waypoints, run this command: `sm_nav_rcbot2_import`

## Common Issues Of RCBot2 Waypoint

RCBot2's waypoint area doesn't translate very well into NavBot's waypoint control point index assignment.    
You may need to review imported waypoints on maps that uses areas (maps with control points, including payload).

## Auto Import of Waypoints

Setting `sm_nav_auto_import` to 2 will enable auto import of waypoints as long a nav mesh was also imported.