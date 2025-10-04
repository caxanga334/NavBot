# Creating new bot types

Like mods, for organization purposes you should create a new folder named after the game/mod you're creating the bot for inside `extension/bot/`.

Using [No More Room in Hell](https://store.steampowered.com/app/224260/No_More_Room_in_Hell/) as an example game, it will be called `nmrih`. Full path is `extension/bot/nmrih/`.

Now inside this new folder, create one header and one source file. Example: `nmrihbot.h` and `nmrihbot.cpp`.

Create a new bot class that derives from `CBaseBot`.

```cpp
class CNMRiHBot : public CBaseBot
{
	CNMRiHBot(edict_t* edict);
	~CNMRiHBot() override;
}
```

Now that the new bot class is created. Go to the mod class and override `CBaseBot* AllocateBot(edict_t* edict)` to return a new instance of the new bot class.

## Joining the game

Periodically bots will try to join the game by calling `void TryJoinGame()`.

You should override this function and do what is necessary to have the bots join the game. (IE: Send the `jointeam` command).

You may also override `bool HasJoinedGame()` and `bool CanJoinGame()`.

# The bot interfaces

The bot has 8 base interfaces: Sensor (`ISensor`), Movement (`IMovement`), Player Controller (`IPlayerController`), Behavior (`IBehavior`), Inventory (`IInventory`), Combat (`ICombat`), Squad (`ISquad`) and Shared Memory (`ISharedBotMemory`).    

## The Sensor Interface

The sensor interface is responsible for the bot's vision, hearing and local memory of entities.    

### Vision Scan

The `ISensor::UpdateKnownEntities` is responsible for updating the bot's vision.    
By default the bot scans for both players and non-player entities.    
Players are scanned in `void ISensor::CollectPlayers(std::vector<edict_t*>& visibleVec)` and non-players in `void ISensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)`.    
Override these functions based on your mod needs. For example, in Black Mesa, the non-player function is overriden to do nothing to safe performance since the bot doesn't need to see non-player entities.

### Identification of Friend or Foe

There are 3 functions responsible for identification of entities by the sensor interface.    
The first function is `bool ISensor::IsIgnored(CBaseEntity* entity) const`, the purpose of this function is to determine if the given entity should be ignored by the bot's sensors.    
Bots will not register ignored entities into the known entity list.    
Then each entity being tracked by the sensor is further filtered by the `bool ISensor::IsEnemy(CBaseEntity* entity) const` and `bool ISensor::IsFriendly(CBaseEntity* entity) const` functions.    
An entity is an enemy to the bot when the `IsEnemy` function returns true.    

### Vision Simulation

The `IsLineOfSightClear` functions uses raycast to determine if there are no vision blocking obstructions between the bot and the given entity or position.    
The `IsInFieldOfView` function checks if the given position is inside the bot's field of view.    
The `IsEntityHidden` and `IsPositionObscured` functions are used to check if a given entity or position is obstructed or hidden from view due to fog, smoke, etc. There is no default implementation for these functions, they always returns false.    
The `IsAbleToSee` is an expensive function that calls all of the functions listed above to determine if a given position or entity is fully visible to the bot.

### Other ISensor Functions

The `int ISensor::GetKnownEntityTeamIndex(CKnownEntity* known)` returns the entity team number for knwon entities instances, the default implementation reads the `m_iTeamNum` member of the entity.    

## The Player Controller Interface

The player controller interface is responsible for the bot's input buttons and mouse movements (view angles).

### Aim Commands

The `AimAt` functions are used send aim commands to the player controller interface.    
The aim target can be a world position coordinate or an entity. If aiming at an entity, the entity position is updated in intervals, the frequency of updates depends on the bot's difficulty.    
Each aim command has a duration in seconds and a priority, if the last aim command hasn't expired and a lower priority command is sent, the command is rejected.    
The `SetDesiredAim*` group of functions sets where the bot should aim at when aiming at entities.    
The `bool IPlayerController::IsAimOnTarget() const` returns true if the bot's is currently looking at the current aim target.    
