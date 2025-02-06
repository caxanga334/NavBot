project "NavBot-EPISODE1"
    language "C++"
    kind "SharedLib"
    cppdialect "C++17"
    targetname "navbot.ext.2.ep1"
    defines { "SOURCE_ENGINE=1" }

    local Dir_SDK = "hl2sdk-episode1"

	includedirs { 
        path.join(Path_HL2SDKROOT, Dir_SDK, "public"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "engine"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "mathlib"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "vstdlib"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "tier0"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "tier1"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "dlls"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "toolframework"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "game", "server"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "game_shared"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "common"),
        path.join(Path_SM, "public"),
        path.join(Path_SM, "public", "extensions"),
        path.join(Path_SM, "sourcepawn", "include"),
        path.join(Path_SM, "public", "amtl", "amtl"),
        path.join(Path_SM, "public", "amtl"),
        path.join(Path_MMS, "core"),
        path.join(Path_MMS, "core", "sourcehook"),
        "../extension",
        "../versioning/include"
	}
	files {
        "../extension/**.h",
        "../extension/**.cpp",
        path.join(Path_SM, "public", "smsdk_ext.cpp"),
	}

    filter { "system:Windows", "architecture:x86" }
        links {
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "mathlib.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "tier0.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "tier1.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "vstdlib.lib"),
        }

    filter { "system:Linux", "architecture:x86" }
        libdirs {
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "linux32")
        }

        -- libs above do not have the lib prefix, so we need to add it
        linkoptions {
            "-l:tier0_i486.so",
            "-l:vstdlib_i486.so",
            "-l:mathlib_i486.a",
            "-l:tier1_i486.a",
        }