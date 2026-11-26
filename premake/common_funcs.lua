
local function _get_path_to_game_shared(sdk_folder)
    local p = path.join(Path_HL2SDKROOT, sdk_folder, "game", "shared")

    if not os.isdir(p) then
        --- EP1/Source 2004
        p = path.join(Path_HL2SDKROOT, sdk_folder, "game_shared")
    end

    return p
end

function include_hl2sdk(sdk_folder)

    includedirs {
        path.join(Path_HL2SDKROOT, sdk_folder, "public"),
        path.join(Path_HL2SDKROOT, sdk_folder, "public", "engine"),
        path.join(Path_HL2SDKROOT, sdk_folder, "public", "mathlib"),
        path.join(Path_HL2SDKROOT, sdk_folder, "public", "vstdlib"),
        path.join(Path_HL2SDKROOT, sdk_folder, "public", "tier0"),
        path.join(Path_HL2SDKROOT, sdk_folder, "public", "tier1"),
        path.join(Path_HL2SDKROOT, sdk_folder, "public", "toolframework"),
        path.join(Path_HL2SDKROOT, sdk_folder, "public", "game", "server"),
        _get_path_to_game_shared(sdk_folder),
        path.join(Path_HL2SDKROOT, sdk_folder, "common"),
    }
end

function include_sm()
    includedirs {
        path.join(Path_SM, "public"),
        path.join(Path_SM, "public", "extensions"),
        path.join(Path_SM, "sourcepawn", "include"),
        path.join(Path_SM, "public", "amtl", "amtl"),
        path.join(Path_SM, "public", "amtl"),
    }
end

function include_mms()
    local p = path.join(Path_MMS, "core", "sourcehook")

    --- If SourceHook folder is not present, include khook
    if not os.isdir(p) then
        p = path.join(Path_MMS, "third_party", "khook", "include")
    end

    includedirs { 
        path.join(Path_MMS, "core"),
        p,
    }
end