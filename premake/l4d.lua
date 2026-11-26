project "NavBot-L4D"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.l4d"
    defines { "SOURCE_ENGINE=13", "NAVBOT_PCH_FILE=\"navbot_pch_l4d.h\""  }

    include_hl2sdk("hl2sdk-l4d")
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
