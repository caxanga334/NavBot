project "NavBot-CSS"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.css"
    defines { "SOURCE_ENGINE=6", "NAVBOT_PCH_FILE=\"navbot_pch_css.h\""  }

    include_hl2sdk("hl2sdk-css")
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
        defines { "CSTRIKE_DLL" }

    filter { "system:Linux" }
        defines { "NO_HOOK_MALLOC", "NO_MALLOC_OVERRIDE" }
