project "NavBot-EPISODE1"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.ep1"
    defines { "SOURCE_ENGINE=1", "NAVBOT_PCH_FILE=\"navbot_pch_episode1.h\""  }

    include_hl2sdk("hl2sdk-episode1")
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
