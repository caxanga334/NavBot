# The Extension Manager

Files: `manager.cpp, manager.h`    
C++ Class: `CExtManager`    
The extension manager is responsible for the storage of bots and players, the mod interface and the bot quota system.

## Access

The extension managed can by accessed by including the `manager.h` header, via the `extmanager` singleton.    

## Mod detection and allocation

The mod detection is handled by the extension manager in the `CExtManager::AllocateMod` function.    

## Getting pointers to players and bots

Each allocated bot and player instance is stored in the extension manager, to access them use the `GetBotBy*` and `GetPlayerBy*` functions.    
You can search via client/entity index or entity pointer.    
Note: Every bot instance is also a player instance but not every player is a NavBot bot!    
If you want to run something on each bot such as call an event function, use the template function `ForEachBot`.

## Adding new NavBots to the game

Call the `CExtManager::AddBot` function to allocate a new fake client and attach a NavBot instance to it.    
The optional `std::string* newbotname` allows you to override the bot name. NULL to use the default name selection.    
The optional `edict_t** newbotedict` allows you to retrieve the `edict_t` of the newly added bot. NULL if the engine failed to create a fake client.    
