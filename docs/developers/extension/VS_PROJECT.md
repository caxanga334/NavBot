# Visual Studio Solution Generation

VS Solutions are generated with [Premake5](https://premake.github.io/).

## Generating The Solution

Open the cloned NavBot folder (where `premake5.lua` is).

Use the following command to generate a solution: `premake5.exe --hl2sdk-root="PATH" --mms-path="PATH" --sm-path="PATH" vs2022`

You need to replace `"PATH"` with the actual paths to MMS, SM and HL2SDK root.

Only Visual Studio 2022 is supported!