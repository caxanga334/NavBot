--
-- premake5 file to build NavBot extension.
-- http://premake.github.io/
--

local action = _ACTION or ""
local todir = "build/" .. action

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

    platforms { 
        "Win32",
        "Win64",
        "Linux32",
        "Linux64"
    }

    location (todir)

    symbols "On"
	exceptionhandling "On"
	rtti "On"
    stringpooling "On"
    omitframepointer "Off"
    vectorextensions "SSE2"
    pic "On"
    include("premake/common_sdk.lua")

    filter { "system:Windows" }
        defines { "_CRT_SECURE_NO_DEPRECATE", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_WINDOWS", "_ITERATOR_DEBUG_LEVEL=0" }
        flags { "MultiProcessorCompile" }
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

    filter { "system:Linux" }
        defines { "LINUX", "_LINUX", "POSIX", "_FILE_OFFSET_BITS=64", "COMPILER_GCC" }
        defines { "stricmp=strcasecmp", "_stricmp=strcasecmp", "_snprintf=snprintf", "_vsnprintf=vsnprintf", "HAVE_STDINT_H", "GNUC" }
        buildoptions { 
            "-Wno-non-virtual-dtor", "-Wno-overloaded-virtual", "-Wno-register", "-Wno-implicit-float-conversion", "-Wno-tautological-overlap-compare",
            "-Wno-inconsistent-missing-override", "-Wno-null-dereference", "-Wno-delete-non-virtual-dtor", "-Wno-sometimes-uninitialized",
            "-Wno-expansion-to-defined", "-Wno-unused", "-Wno-switch", "-Wno-array-bounds", "-Wno-varargs", "-Wno-implicit-exception-spec-mismatch",
        }
        -- disable prefixes for Linux
        targetprefix ""

    filter { "configurations:Debug" }
        defines { "EXT_DEBUG" }
        targetdir "build/bin/%{cfg.architecture}/debug"
        optimize "Off"

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        targetdir "build/bin/%{cfg.architecture}/release"
        optimize "Full"
        flags { "LinkTimeOptimization" }
		visibility "Hidden"

    filter { "platforms:Win32" }
        system "Windows"
        architecture "x86"
        defines { "WIN32", "COMPILER_MSVC", "COMPILER_MSVC32" }

    filter { "platforms:Win64" }
        system "Windows"
        architecture "x86_64"
        defines { "WIN64", "COMPILER_MSVC", "COMPILER_MSVC64", "X64BITS", "PLATFORM_64BITS" }

    filter { "platforms:Linux32" }
        system "Linux"
        architecture "x86"

    filter { "platforms:Linux64" }
        system "Linux"
        architecture "x86_64"
        defines { "X64BITS", "PLATFORM_64BITS" }

include("premake/tf2.lua")
include("premake/dods.lua")
include("premake/css.lua")
include("premake/bms.lua")
include("premake/sdk2013.lua")
include("premake/hl2dm.lua")
include("premake/orangebox.lua")