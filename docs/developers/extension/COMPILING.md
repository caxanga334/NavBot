# Compiling NavBot

[SourceMod]: https://github.com/alliedmodders/sourcemod
[MetaMod: Source]: https://github.com/alliedmodders/metamod-source
[HL2 SDKs]: https://github.com/alliedmodders/hl2sdk
[AMBuild]: https://github.com/alliedmodders/ambuild
[CI]: https://github.com/caxanga334/NavBot/blob/main/.github/workflows/build-action.yml

## Prerequisites

In order to compile NavBot, you need:

* A C++ compiler:
  * Windows: MSVC (Visual Studio).
  * Linux: Clang or GCC. Clang is preferred.
* Python >= 3.8
* [AMBuild]
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

git clone https://github.com/alliedmodders/sourcemod.git sourcemod-1.13 --recurse-submodules --depth=1
git clone https://github.com/alliedmodders/sourcemod.git sourcemod-1.12 --recurse-submodules --depth=1 --branch=1.12-dev
git clone https://github.com/alliedmodders/sourcemod.git sourcemod-1.11 --recurse-submodules --depth=1 --branch=1.11-dev
git clone https://github.com/alliedmodders/metamod-source.git mmsource-2.0 --recurse-submodules --depth=1
git clone https://github.com/alliedmodders/metamod-source.git mmsource-1.12 --recurse-submodules --depth=1 --branch=1.12-dev
git clone https://github.com/alliedmodders/metamod-source.git mmsource-1.11 --recurse-submodules --depth=1 --branch=1.11-dev
```

## Building

* Open terminal (linux) or Developer powershell/command prompt (Windows)
* Clone NavBot: `git clone https://github.com/caxanga334/NavBot.git --recurse-submodules`
* Open the NavBot folder.
* Create a new folder called `build`.
* Enter the `build` folder.
* Configure the folder for AMBuild: `python ..\configure.py --hl2sdk-root {PATH} --mms-path {PATH} --sm-path {PATH}`
* Then type `ambuild` to compile.

For a full example, see how [CI] builds NavBot.

### Configure.py Parameter list

A full list can be retrieved with `--help`. (This will include AMBuild built-in parameters).

* `--hl2sdk-root`: Path to the root folder that contains the copies of HL2-SDK.
* `--mms-path`: Path to Metamod: Source.
* `--sm-path`: Path to SourceMod.
* `--targets`: Override the target architecture (use commas to separate multiple targets).
* `--arch-options`: x86 architecture options.
  * 0 for default
  * 1 for SSE4.2
  * 2 for AVX2
  * 3 for target native CPU (Linux only)

