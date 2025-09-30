# Requirements

- Sourcemod 1.12 or better
- Metamod: Source 1.12 or better

# Downloading

## Development Builds

The latest dev builds of NavBot are available as workflow artifacts. Check the [Actions] page.    
Sometimes dev builds are pushed as pre-releases. Check the [Releases] page for the latest pre-release.

## Stable Builds

NavBot is currently in development and does not have stable releases.    
They will be available on the [Releases] in the future.

# Installing

Copy the extension files (extension, plugins, gamedata, data, translations and config) files into your sourcemod install directory.    
Generally it's `<game root>/addons/sourcemod/`.

<!-- LINKS -->

[Actions]: https://github.com/caxanga334/NavBot/actions
[Releases]: https://github.com/caxanga334/NavBot/releases

## Setting Up A Listen Server

A listen server is required if you intent on editing the navigation mesh or wants to use the built-in debug commands.    
The basic installation of Metamod and Sourcemod is the same for a listen server.    
To run the game with MM and SM, you need to launch the game in insecure mode by adding `-insecure` to the launch parameters.    

### Windows Listen Server

I recommend creating a batch script to run the game instead of modifying the Steam launch options for the game.    
Create a **.bat** file on the same folder where the *.exe* of the game is, then add the following:  

```
start <exe> -steam -game <game folder> -console -insecure -dev -condebug
```

* <exe> : Replace with the game's launcher executable.
* <game folder> : Replace with the game's game folder name.

Example for Team Fortress 2:    

```
start tf_win64.exe -steam -game tf -console -insecure -dev -condebug
```

Save the file and double click it to launch the game, Steam needs to be running.    
The game will run in insecure and developer mode.    
Note: You need to load a map before checking if SM and MM:S is installed and working correctly.

### Linux Listen Server

Currently not supported, install for Windows and run the game via Proton.    
