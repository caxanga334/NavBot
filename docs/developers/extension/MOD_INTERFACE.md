# The Mod Interface

The mod interface is responsible for mod specific stuff such as game mode detection, bot class allocation, nav mesh class allocation, etc.    
The mod settings, weapon info manager, difficulty profile manager and shared bot memory interfaces are managed by the mod interface.    

## Accessing the mod interface

The mod interface is stored in the extension manager. To access it, include `manager.h` and `yourmod.h`, then use `extmanager->GetMod()` to get a pointer to the mod interface.    

## Mod name and ID

The functions `GetModName` and `GetModType` should be overriden to return the mod name and ID. If needed, add a new mod ID to the `Mods::ModType` enum in `extension\mods\gamemods_shared.h`.    

## Mod bot allocation

The `AllocateBot` function is called by the extension manager when creating bots and it should be overriden to allocate a new instance of the mod specific bot class.    

## Mod specific nav mesh implementation

If your mod needs a custom implementation of the nav mesh, override `NavMeshFactory` in your mod class to allocate an instace of your mod specific nav mesh class instead of the base class.

## Listening to game events

The mod interface implements the Source SDK's `IGameEventListener2` interface.    
To listen for a game event, call `ListenForGameEvent(const char* name)`. This function only needs to be called once. The constructor is a good place to call it.    
Then the engine will invoke the `void FireGameEvent(IGameEvent* event)` each time a event that's being listen is fired.    
Example:

```cpp

CMyMod::CMyMod()
{
    ListenForGameEvent("round_start");
    ListenForGameEvent("round_end");
    ListenForGameEvent("foobar");
}

void CMyMod::FireGameEvent(IGameEvent* event)
{
    // The engine accepts NULL events, so this check is needed to avoid a crash.
    if (!event) { return; }

    if (std::strcmp(event->GetName(), "round_start") == 0)
    {
        // "round_start" event was fired, do stuff
        OnRoundStart(); // Call the round start function
        ...
    }
}

```

## Drive functions

The `CBaseMod::Frame` function is called on every server frame, avoid doing expensive stuff here.   
The `CBaseMod::Update` function is called once every one second, do any heavyweight stuff here.    
Note: When overriding these functions, you should always call the base class functions.    

## Notification functions

The `CBaseMod::OnMapStart` function is called by Sourcemod when a new map is loaded.    
The `CBaseMod::OnMapEnd` function is called by Sourcemod when the current map is unloading.    
The `CBaseMod::OnNavMeshLoaded` function is called by the navigation mesh to notify the mod interface it just finished loading.    
The `CBaseMod::OnNavMeshDestroyed` function is called to notify the mod that the navigation mesh was destroyed and any pointers to nav mesh objects are now invalid.    

## Client commands

NavBot hooks the engine's client command funcion. This hook calls the `CBaseMod::OnClientCommand` function.    
