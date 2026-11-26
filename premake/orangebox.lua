project "NavBot-ORANGEBOX"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.ep2"
    defines { "SOURCE_ENGINE=3", "NAVBOT_PCH_FILE=\"navbot_pch_orangebox.h\""  }

    include_hl2sdk("hl2sdk-orangebox")
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

    filter { "system:Windows", "architecture:x86" }
        links {
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "mathlib.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "tier0.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "tier1.lib"),
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "public", "vstdlib.lib"),
        }

    filter { "system:Linux", "architecture:x86" }
        libdirs {
            path.join(Path_HL2SDKROOT, Dir_SDK, "lib", "linux")
        }

        -- libs above do not have the lib prefix, so we need to add it
        linkoptions {
            "-l:tier0_i486.so",
            "-l:vstdlib_i486.so",
            "-l:mathlib_i486.a",
            "-l:tier1_i486.a",
        }