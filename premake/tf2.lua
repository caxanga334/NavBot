project "NavBot-TF2"
    language "C++"
    kind "SharedLib"
    cppdialect "C++20"
    targetname "navbot.ext.2.tf2"
    defines { "SOURCE_ENGINE=12", "NAVBOT_PCH_FILE=\"navbot_pch_tf2.h\"" }

    include_hl2sdk("hl2sdk-tf2")
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
        defines { "TF_DLL" }
