project "NavBot-BMS"
    language "C++"
    kind "SharedLib"
    cppdialect "C++17"
    targetname "navbot.ext.2.bms"
    defines { "SOURCE_ENGINE=11", "NAVBOT_PCH_FILE=\"navbot_pch_bms.h\"" }

    local Dir_SDK = "hl2sdk-bms"

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
        "../versioning/include"
	}
	files {
        "../extension/**.h",
        "../extension/**.cpp",
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