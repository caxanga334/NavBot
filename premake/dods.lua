project "NavBot-DODS"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.dods"
    defines { "SOURCE_ENGINE=8", "NAVBOT_PCH_FILE=\"navbot_pch_dods.h\"" }

    include_hl2sdk("hl2sdk-dods")
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
        defines { "DOD_DLL" }
