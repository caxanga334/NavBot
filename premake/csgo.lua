project "NavBot-CSGO"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.csgo"
    defines { "SOURCE_ENGINE=23", "NAVBOT_PCH_FILE=\"navbot_pch_csgo.h\""  }

    include_hl2sdk("hl2sdk-csgo")
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

    filter {}
        defines { "_GLIBCXX_USE_CXX11_ABI=0" }