project "NavBot-L4D2"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.l4d2"
    defines { "SOURCE_ENGINE=16", "NAVBOT_PCH_FILE=\"navbot_pch_l4d2.h\""  }

    include_hl2sdk("hl2sdk-l4d2")
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
        defines { "TERROR" }
