# How to add support to a new game/mod

This page explains how to add support for a new game/mod. As an example, we will use [No More Room in Hell](https://store.steampowered.com/app/224260/No_More_Room_in_Hell/).

To begin adding support to a new game/mod, begin by creating the needed folders. I like to name them after the mod/game's name.

Go to `extension/mods/`, create a new folder for the game/mod. In our example it will be named `nmrih`.

Inside this new folder, create a header and source file. Example: `nmrihmod.h` and `nmrihmod.cpp`.

Define a new mod class. Generally you will use `CBaseMod` as a base class. In our example for NMRiH, `CBaseMod` will be used as the base class.

```cpp

class CNoMoreRoomInHellMod : public CBaseMod
{
public:
	CNoMoreRoomInHellMod() : CBaseMod() {}
	~CNoMoreRoomInHellMod() override {}

	const char* GetModName() override { return "No More Room In Hell"; }
	Mods::ModType GetModType() override { return Mods::ModType::MOD_NMRIH; }
};

```

We now have our mod class and also have setup the mod name and mod ID. Your game/mod probably doesn't exists in the ModType enum, add it at the end of the list.

# Allocating your mod

Now we need to tell the manager to allocate a new instance of the NMRiH mod.

This is done at `void CExtManager::AllocateMod`.

Because NMRiH runs on the Source SDK 2013 branch, we will need to detect if the current game/mod is NMRiH. The most simple way of doing this is by checking the name of the current game folder.

```cpp
const char* gamefolder = smutils->GetGameFolderName();

if (strncasecmp(gamefolder, "nmrih", 5) == 0)
{
    m_mod = std::make_unique<CNoMoreRoomInHellMod>();
}
```

Now compile navbot and watch the log files.

Remember to regenerate the project with premake5 to include any additing header and source files you created.