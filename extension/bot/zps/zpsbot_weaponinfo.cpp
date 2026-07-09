#include NAVBOT_PCH_FILE
#include "zpsbot_weaponinfo.h"

SourceMod::SMCResult CZPSWeaponInfoManager::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
    if (m_parser_depth == PARSER_DEPTH_WEAPONDATA)
    {
        if (std::strcmp(name, "zps_data") == 0)
        {
            m_inzpsdatasection = true;
            m_parser_depth++;
            return SourceMod::SMCResult::SMCResult_Continue;
        }
    }

    return CWeaponInfoManager::ReadSMC_NewSection(states, name);
}

SourceMod::SMCResult CZPSWeaponInfoManager::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
    if (!m_inzpsdatasection)
    {
        return CWeaponInfoManager::ReadSMC_KeyValue(states, key, value);
    }

    if (std::strcmp(key, "size_in_inventory") == 0)
    {
        int size = ParseInt(states, value, 1);
        size = std::max(size, 0);
        GetCurrentZPSInfo()->SetInventorySize(size);
    }
    else
    {
        smutils->LogError(myself, "[ZPS Weapon Info] Unknown Key Value pair \"%s\" \"%s\" at line %u col %u!", key, value, states->line, states->col);
    }

    return SourceMod::SMCResult::SMCResult_Continue;
}

SourceMod::SMCResult CZPSWeaponInfoManager::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
    if (m_inzpsdatasection)
    {
        m_inzpsdatasection = false;
    }

    return CWeaponInfoManager::ReadSMC_LeavingSection(states);
}
