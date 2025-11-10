project "NavBot-SWARM"
    language "C++"
    kind "SharedLib"
    cppdialect "C++17"
    targetname "navbot.ext.2.swarm"
    defines { "SOURCE_ENGINE=17", "NAVBOT_PCH_FILE=\"navbot_pch_swarm.h\""  }

    local Dir_SDK = "hl2sdk-swarm"

	includedirs { 
        path.join(Path_HL2SDKROOT, Dir_SDK, "public"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "engine"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "mathlib"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "vstdlib"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "tier0"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "tier1"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "toolframework"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "game", "server"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "game", "shared"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "common"),
        path.join(Path_SM, "public"),
        path.join(Path_SM, "public", "extensions"),
        path.join(Path_SM, "sourcepawn", "include"),
        path.join(Path_SM, "public", "amtl", "amtl"),
        path.join(Path_SM, "public", "amtl"),
        path.join(Path_MMS, "core"),
        path.join(Path_MMS, "core", "sourcehook"),
        "../build/navbot_pch_*/**",
        "../extension",
        "../versioning/include",
        "../build/includes",
	}
	files {
        "../extension/**.h",
        "../extension/**.cpp",
        "../build/includes/**.h",
        path.join(Path_SM, "public", "smsdk_ext.cpp"),
	}

    -- reset filter    
    filter {}
        defines { "INFESTED_DLL" }

    filter { "system:Windows", "architecture:x86" }
        links {
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "mathlib.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "tier0.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "tier1.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "vstdlib.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "interfaces.lib"),
        }

    filter {}
        defines { "_GLIBCXX_USE_CXX11_ABI=0" }