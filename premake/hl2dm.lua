project "NavBot-HL2DM"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.hl2dm"
    defines { "SOURCE_ENGINE=7", "NAVBOT_PCH_FILE=\"navbot_pch_hl2dm.h\""  }

    include_hl2sdk("hl2sdk-hl2dm")
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

