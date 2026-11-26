project "NavBot-SWARM"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.swarm"
    defines { "SOURCE_ENGINE=17", "NAVBOT_PCH_FILE=\"navbot_pch_swarm.h\""  }

    include_hl2sdk("hl2sdk-swarm")
    include_sm()
    include_mms()

	includedirs { 
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