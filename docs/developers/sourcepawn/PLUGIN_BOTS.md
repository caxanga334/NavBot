# Plugin Bots

NavBot instances can be attached to existing bots and human players.

This will create a new Plugin Bot instance.

Currently the plugin bot is very limited.

## Attaching a Plugin Bot

Plugin Bot instances are created via the NavBot methodmap constructor.

Example:

`NavBot navbot = NavBot(client);`

**client** must be a valid client index.

## Checking For Plugin Bots

Use `NavBot bot = GetNavBotByIndex(int client)` to get a NavBot instance from the given client index.

To check if it's a valid NavBot instance, use `bot.IsNull`.

If you have a valid NavBot instance, use `bot.IsPluginBot()` to check if this NavBot instance was created by a plugin.

# Mesh Navigator

The `MeshNavigator` is used for navigating NavBot's navigation mesh.

This navigator only works with Plugin Bot instances.

## Creating a New Mesh Navigator

`MeshNavigator nav = MeshNavigator();`

This will allocate a new Mesh Navigator instance.

## Destroying The Navigator

```cpp
MeshNavigator nav = MeshNavigator();
nav.Destroy();
```

The `Destroy` native will delete the navigator object from memory.

After destroy, it's recommended to set the navigator variable to `NULL_MESHNAVIGATOR`.

**Important!** All mesh navigators are automatically destroyed on map end. You should set all navigator variables to `NULL_MESHNAVIGATOR` on map end.

## Computing a Path

The `ComputeToPos` function can be used to find a path for a given bot.

It will return true if a path is found, otherwise it will return false.

Use `IsValid` to see if a valid path exists.

Paths should be recomputed at least once per second.

## Using The Path

Call the navigator's `Update` function to have the bot follow the path.

Note that the bot doesn't move automatically, use `GetMoveGoal` to get the move goal position from the navigator and manually make the bot move towards it.


# Example Plugin

The [NavBot example plugin] contains an example of how to make humans path find to a destination.


[NavBot example plugin]: https://github.com/caxanga334/NavBot/blob/main/scripting/navbot_example.sp