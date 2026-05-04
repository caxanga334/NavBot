# Troubleshooting

Common issues you may encounter and possible solutions.

## Extension Not Loading

First, check if SM is loaded by typing `sm version` in the server console.    
If you get `unknown command 'sm'` then Sourcemod itself is not loading/installed correctly.    
If Sourcemod is loaded, check there is anything in the error logs.    

## Bots Doesn't Move

A common issues is either a missing or bad nav mesh. Run the `sm_navbot_info` command, it will output a bunch of information to the console, one of them is the Nav Mesh status.
If it says the nav mesh is **NOT Loaded**, then a nav mesh is missing for the current map.    
If a nav mesh is present, then it could be an unsupported game or game mode.    
It could also be a bad/incomplete nav mesh.    

## Debugging Crashes

Install and configure [Accelerator](https://forums.alliedmods.net/showthread.php?t=277703&).    
Accelerator doesn't catch every type of crash. If you're experiencing crashes but the accelerator logs are empty, check the Steam's dump folder if you're running on a Windows Listen Server. (Default path is `C:\Program Files (x86)\Steam\dumps`). If you're running on a Dedicated Server, attach a debugger.    