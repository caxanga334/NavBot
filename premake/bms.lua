project "NavBot-BMS"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.bms"
    defines { "SOURCE_ENGINE=11", "NAVBOT_PCH_FILE=\"navbot_pch_bms.h\"" }

    include_hl2sdk("hl2sdk-bms")
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
        defines { "HL2_DLL", "HL2MP" }

    filter { "system:Linux" }
        defines { "NO_HOOK_MALLOC", "NO_MALLOC_OVERRIDE" }

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
            "-l:libtier0_srv.so",
            "-l:libvstdlib_srv.so",
            "-l:mathlib.a",
            "-l:tier1.a",
        }