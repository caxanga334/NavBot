--
-- premake5 file to build NavBot extension.
-- http://premake.github.io/
--

local action = _ACTION or ""
local todir = "premake_build/" .. action

newoption {
    trigger     = "hl2sdk-root",
    value       = "absolute path",
    description = "Absolute path to the root folter that contains the HL2-SDKs",
    category    = "Paths"
}

newoption {
    trigger     = "mms-path",
    value       = "absolute path",
    description = "Absolute path to the root metamod-source folder",
    category    = "Paths"
}

newoption {
    trigger     = "sm-path",
    value       = "absolute path",
    description = "Absolute path to the root sourcemod folder",
    category    = "Paths"
}

newoption {
    trigger     = "instructions-set",
    value       = "empty or sse2 or sse4 or avx2",
    description = "Allows the compiler to use SSE4 or AVX2 instructions. SSE2 is the default.",
    category    = "compilers"
}

newoption {
    trigger     = "valve-prof",
    description = "Enables VPROF.",
    category    = "BuildOptions"
}

newoption {
    trigger     = "no-sourcepawn-api",
    description = "Disables the SourcePawn API.",
    category    = "BuildOptions"
}


Path_HL2SDKROOT = ""
Path_SM = ""
Path_MMS = ""

-- when "premake5 --help" is ran, _ACTION is NULL (nil)
if (_ACTION ~= nil) then
    if (_OPTIONS["hl2sdk-root"] == nil) then
        error("Failed to get path to HL2SDKs. Set path with --hl2sdk-root=path")
    end

    if (_OPTIONS["mms-path"] == nil) then
        error("Failed to get path to Metamod Source. Set path with --mms-path=path")
    end

    if (_OPTIONS["sm-path"] == nil) then
        error("Failed to get path to SourceMod. Set path with --sm-path=path")
    end

    if (not os.isdir(_OPTIONS["hl2sdk-root"])) then
        error("Failed to get path to HL2SDKs. Set path with --hl2sdk-root=path")
    end

    if (not os.isdir(_OPTIONS["mms-path"])) then
        error("Failed to get path to Metamod Source. Set path with --mms-path=path")
    end

    if (not os.isdir(_OPTIONS["sm-path"])) then
        error("Failed to get path to SourceMod. Set path with --sm-path=path")
    end

    Path_HL2SDKROOT = path.normalize(_OPTIONS["hl2sdk-root"])
    Path_SM = path.normalize(_OPTIONS["sm-path"])
    Path_MMS = path.normalize(_OPTIONS["mms-path"])
end

workspace "navbot"
    configurations { 
        "Debug",
        "Release"
    }

    if os.host() == "windows" then
        platforms { 
            "Win32",
            "Win64"
        }
    end

    if os.host() == "linux" then
        platforms { 
            "Linux32",
            "Linux64"
        }
    end

    location (todir)

    symbols "On"
    exceptionhandling "On"
    rtti "On"
    stringpooling "On"
    omitframepointer "Off"
    vectorextensions "SSE2"
    pic "On"
    include("premake/common_sdk.lua")

    filter { "options:instructions-set=avx2" }
        vectorextensions "AVX2"

    filter { "options:instructions-set=sse4" }
        vectorextensions "SSE4.2"
    
    filter { "options:valve-prof" }
        defines { "EXT_VPROF_ENABLED" }

    filter { "options:no-sourcepawn-api" }
        defines { "NO_SOURCEPAWN_API" }

    filter { "system:Windows" }
        toolset "msc"
        clangtidy("On")
        runcodeanalysis("On")
        defines { "_CRT_SECURE_NO_DEPRECATE", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_WINDOWS", "_ITERATOR_DEBUG_LEVEL=0" }
        multiprocessorcompile "on"
        characterset "MBCS"
        staticruntime "on"
        runtime "Release"
        editandcontinue "off"
        -- Disable macro redefinition warnings
        disablewarnings { "4005"}
        links {
            "kernel32.lib",
            "user32.lib",
            "legacy_stdio_definitions.lib",
        }

    -- Linux options shared for both GCC/Clang
    filter { "system:Linux" }
        toolset "clang"
        defines { "LINUX", "_LINUX", "POSIX", "_FILE_OFFSET_BITS=64", "COMPILER_GCC" }
        defines { "stricmp=strcasecmp", "_stricmp=strcasecmp", "_snprintf=snprintf", "_vsnprintf=vsnprintf", "HAVE_STDINT_H", "GNUC" }
        buildoptions { 
            "-Wno-non-virtual-dtor", "-Wno-overloaded-virtual", "-Wno-register", "-Wno-varargs", "-Wno-array-bounds", "-Wno-unused",
            "-Wno-null-dereference", "-Wno-delete-non-virtual-dtor", "-Wno-switch", "-Wno-expansion-to-defined",  
        }
        -- disable prefixes for Linux
        targetprefix ""
        
        if os.host() == "linux" then
            visibility "Hidden"
        end

    -- Linux options for Clang only
    filter { "system:linux", "toolset:clang" }
        buildoptions {
            "-Wno-implicit-exception-spec-mismatch", "-Wno-sometimes-uninitialized", "-Wno-inconsistent-missing-override", "-Wno-tautological-overlap-compare",
            "-Wno-implicit-float-conversion",
        }

    filter {}
        defines { "NOMINMAX" }

    filter { "configurations:Debug" }
        defines { "EXT_DEBUG" }
        targetdir "build/bin/%{cfg.architecture}/debug"
        optimize "Off"

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        targetdir "build/bin/%{cfg.architecture}/release"
        optimize "Full"
        linktimeoptimization "On"

    filter { "platforms:Win32" }
        system "Windows"
        architecture "x86"
        defines { "WIN32", "COMPILER_MSVC", "COMPILER_MSVC32" }

    filter { "platforms:Win64" }
        system "Windows"
        architecture "x86_64"
        defines { "WIN32", "WIN64", "COMPILER_MSVC", "COMPILER_MSVC64", "X64BITS" }

    filter { "platforms:Linux32" }
        system "Linux"
        architecture "x86"

    filter { "platforms:Linux64" }
        system "Linux"
        architecture "x86_64"
        defines { "X64BITS" }

    -- reset filter    
    filter {}

include("premake/tf2.lua")
include("premake/dods.lua")
include("premake/css.lua")
include("premake/csgo.lua")
include("premake/bms.lua")
include("premake/sdk2013.lua")
include("premake/hl2dm.lua")
include("premake/orangebox.lua")
include("premake/episode1.lua")
include("premake/l4d.lua")
include("premake/l4d2.lua")
include("premake/swarm.lua")