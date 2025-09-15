# Troubleshooting

Common issues you may encounter and possible solutions.

## Extension Not Loading

First, check if SM is loaded by typing `sm version` in the server console.    
If you get `unknown command 'sm'` then Sourcemod itself is not loading/installed correctly.    
If Sourcemod is loaded, check there is anything in the error logs.    

## Bots Doesn't Move

If the bots aren't moving, run the `sm_navbot_info` command. This command will output a bunch of information, one of them is the Nav Mesh status.    
If the Nav Mesh status is `NOT Loaded`, then your map is missing a nav mesh.    
If it's `Loaded`, then it could either be an unsupported mod or unsupported game mode.    

## Debugging Crashes

Install and configure [Accelerator](https://forums.alliedmods.net/showthread.php?t=277703&).    
If you're on x64, you need to either manually compile [Accelerator](https://github.com/asherkin/accelerator) or download the latest build artifact from [here](https://github.com/asherkin/accelerator/actions) if available.    
Accelerator doesn't catch every type of crash. If you're experiencing crashes but the accelerator logs are empty, check the Steam's dump folder if you're running on a Windows Listen Server. (Default path is `C:\Program Files (x86)\Steam\dumps`). If you're running on a Dedicated Server, attach a debugger.    