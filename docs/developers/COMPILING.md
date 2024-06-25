# Compiling NavBot

[SourceMod]: https://github.com/alliedmodders/sourcemod
[MetaMod: Source]: https://github.com/alliedmodders/metamod-source
[HL2 SDKs]: https://github.com/alliedmodders/hl2sdk
[Premake5]: https://premake.github.io/
[CI]: https://github.com/caxanga334/NavBot/blob/main/.github/workflows/build-action.yml

## Prerequisites

In order to compile NavBot, you need:

* A C++ compiler
  * Windows: MSVC (Visual Studio)
  * Linux: Clang
* [Premake5]
* Git

## Setup

First you need to clone [SourceMod], [MetaMod: Source] and the [HL2 SDKs].

You can use the following bash script to do this:

On Windows, run these scripts via git bash.

```bash
#!/bin/bash

sdks=(tf2 css csgo hl2dm dods orangebox sdk2013 bms l4d l4d2 eye swarm portal2 pvkii nucleardawn mcv insurgency dota cs2 contagion blade bgt darkm doi episode1)
for sdk in "${sdks[@]}"
do
	git clone https://github.com/alliedmodders/hl2sdk --branch=$sdk --recurse-submodules --depth=1 hl2sdk-$sdk
done

git clone https://github.com/alliedmodders/hl2sdk-mock.git hl2sdk-mock --recurse-submodules --depth=1
```

```bash
#!/bin/bash

git clone https://github.com/alliedmodders/sourcemod.git sourcemod-1.12 --recurse-submodules --depth=1
git clone https://github.com/alliedmodders/sourcemod.git sourcemod-1.11 --recurse-submodules --depth=1 --branch=1.11-dev
git clone https://github.com/alliedmodders/metamod-source.git mmsource-2.0 --recurse-submodules --depth=1
git clone https://github.com/alliedmodders/metamod-source.git mmsource-1.12 --recurse-submodules --depth=1 --branch=1.12-dev
git clone https://github.com/alliedmodders/metamod-source.git mmsource-1.11 --recurse-submodules --depth=1 --branch=1.11-dev
```

Then clone NavBot.

Now you need to generate the project for your system using premake5.

Go to the navbot folder where **premake5.lua** is and run premake5 to generate it.

You need to pass a few parameters to premake5:

* --hl2sdk-root: Full path to the folder where the hl2sdk-* folders.
* --mms-path: Full path to the Metamod Source folder.
* --sm-path: Full path to the Sourcemod folder.

Windows Example: `premake5.exe --hl2sdk-root="C:\alliedmodders" --mms-path="C:\alliedmodders\mmsource-2.0" --sm-path="C:\alliedmodders\sourcemod-1.12" vs2022`

Linux Example: `premake5 --hl2sdk-root="/home/user/alliedmodders" --mms-path="/home/user/alliedmodders/mmsource-2.0" --sm-path="/home/user/alliedmodders/sourcemod-1.12" gmake2`

A new folder called `build` will be generated, inside it there will be another folder named after the action choosen for premake5. If generating a Visual Studio 2022 solution, it will be called `vs2022`. If generating a makefile, it will be `gmake2`.

On Windows, you can now compile by opening the solution in VS and compiling it from there or use MSBuild.

On Linux, you can now compile by running make with the correct configuration.

For a full example, see how [CI] builds NavBot.