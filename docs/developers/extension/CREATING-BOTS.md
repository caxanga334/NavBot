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

## Interfaces

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

You can also create new mod specific interfaces to if needed. See `CTF2BotSpyMonitor` as an example.

## Tasks

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