# Nav Bot Developer Documentation

## Adding Support To a New Mod

Begin by creating a new folder that provides a good description of the mod (Example: For Zombie Panic Source, create a folder named 'zps', using a folder with the same name as the mod 'gamefolder' is also a good start).

Create a new mod class that derives from `CBaseMod`. Make sure to give it a fitting name. (Using Zombie Panic Source as an example, `CZPSMod`).

There are a few functions that every mod should override.

- GetModName()
- GetModType()

The `enum ModType` won't have all source mods, add a unique value for your mod if it doesn't exists.

Now you need to tell the extension manager to detect your mod and allocate it.

The mod object is allocated by `CExtManager::AllocateMod()` located in `manager.cpp`.

The most basic form of mod detection is via game folder name.

Example:

```cpp
const char* gamefolder = smutils->GetGameFolderName();

if (strncasecmp(gamefolder, "zps", 3) == 0)
{
    m_mod = std::make_unique<CZPSMod>();
}
```

This should cover the basic for the mod part. You will need to override other functions in the future.

## Creating Your New Bot

Begin by creating a new bot class that derives from `CBaseBot`.

Let's begin with something basic: Joining the game.

The function `virtual bool CanJoinGame()` is used to check if the bot is able to join the game. It should return true when the bot is able to join the game.

The function `virtual bool HasJoinedGame()` is used to check if the has already joined the game. It should return false if the bot still needs to join the game.

The function `virtual void TryJoinGame()` is called when trying to join the game. If `HasJoinedGame()` returns false and `CanJoinGame()` returns true, then it will be called.
Place whatever is necessary for the bot to join the game in this function. See `void CTF2Bot::TryJoinGame()` for an example.

### Interfaces

The bot has 4 base interfaces. Player Controller, Sensor, Movement, Behavior.

Player Controller (`IPlayerController`) is responsible for the bot's inputs and aiming.

Sensor (`ISensor`) is responsible for the bot senses. It currently handles vision and hearing.

Movement (`IMovement`) is responsible for moving the bot. It sends the necessary inputs to the player controller ir order for the bot to move towards a given position.
It also provides a description of the bot movement capabilities for for pathfinder.

Behavior (`IBehavior`) is responsible for the bot 'brain'. It handles the decision queries (`IDecisionQuery`) and the task system (`AITaskManager<Class>`).

At minimum, you need to declare a new behavior interface for the new bot.
You will also need to write new tasks for the bot, this will be explained later.

A Sensor interface is also needed, since it handles what is enemy and what is ally for the bot.

Movement may be needed if the mod movement differs from the HL2 movement.


### Tasks

The task system consist of a Hierarchical Finite State Machine combined with Stack.

To create a new task, derive from `AITask<YourBotClass>`.

Tasks have a lot of functions that you can override, I recommend reading the `tasks.h` file.

Let's begin with a simple example.

The function `virtual TaskResult<BotClass> OnTaskUpdate(BotClass* bot)` is called while the task is being updated (active and running).
This is where you would code the work the bot needs to do to complete that task.

Each task must return a task result object, you don't create these objects manually, instead you use these functions.

```cpp
// Keep the current task running
TaskResult<BotClass> Continue() const;
// Switch to a new task, the current task will be destroyed
TaskResult<BotClass> SwitchTo(AITask<BotClass>* newTask, const char* reason) const;
// Put the current task on pause and begin running another one on it's place, the current task will be kept and returned to when the new task ends
TaskResult<BotClass> PauseFor(AITask<BotClass>* newTask, const char* reason) const;
// Ends and destroy the current task
TaskResult<BotClass> Done(const char* reason) const;
```

There are four type of results for a task.

`CONTINUE` will keep executing the current task.

`SWITCH` will end and destroy the current task and start another task on it's place.

`PAUSE` will pause the current task and start another task on it's place, the old task is placed below the new task. When the new task ends, the old task is resumed.

`DONE` will end and destroy the current task, any task below it will be resumed.