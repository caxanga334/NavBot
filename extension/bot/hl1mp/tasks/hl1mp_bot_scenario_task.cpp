#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <navmesh/nav_mesh.h>
#include <bot/hl1mp/hl1mp_bot.h>
#include <bot/tasks_shared/bot_shared_simple_deathmatch_behavior.h>
#include <bot/tasks_shared/bot_shared_collect_items.h>
#include "hl1mp_bot_scenario_task.h"

class CHL1MPPickupWeaponFilter : public NBotSharedCollectItemTask::ItemCollectFilter<CHL1MPBot, CNavArea>
{
public:
    CHL1MPPickupWeaponFilter(CHL1MPBot* bot) :
        NBotSharedCollectItemTask::ItemCollectFilter<CHL1MPBot, CNavArea>(bot)
    {
    }

    bool IsItemValid(CBaseEntity* object, CNavArea* navarea) override
    {
        if (NBotSharedCollectItemTask::ItemCollectFilter<CHL1MPBot, CNavArea>::IsItemValid(object, navarea))
        {
            CBaseEntity* owner = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(object, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER)->Get();

            // can't pickup weapons owned by someone
            if (owner)
            {
                return false;
            }

            return true;
        }

        return false;
    }
};

static bool PickupWeaponValidatorFunc(CHL1MPBot* bot, CBaseEntity* entity)
{
    if (entityprops::IsEffectActiveOnEntity(entity, EF_NODRAW))
    {
        return false;
    }

    CBaseEntity* owner = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(entity, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER)->Get();

    if (owner)
    {
        return false;
    }

    const char* classname = gamehelpers->GetEntityClassname(entity);

    // already owns it
    if (bot->GetInventoryInterface()->FindWeaponByClassname(classname) != nullptr)
    {
        return false;
    }

    return true;
}

static bool PickupLongJumpValidatorFunc(CHL1MPBot* bot, CBaseEntity* entity)
{
    if (entityprops::IsEffectActiveOnEntity(entity, EF_NODRAW))
    {
        return false;
    }

    if (bot->HasLongJumpModule())
    {
        return false;
    }

    return true;
}

AITask<CHL1MPBot>* CHL1MPBotScenarioTask::InitialNextTask(CHL1MPBot* bot)
{
    return new CBotSharedSimpleDMBehaviorTask<CHL1MPBot, CHL1MPBotPathCost>();
}

TaskResult<CHL1MPBot> CHL1MPBotScenarioTask::OnTaskStart(CHL1MPBot* bot, AITask<CHL1MPBot>* pastTask)
{
    m_pickupWeaponsTimer.StartRandom(2.0f, 15.0f);
    m_pickupAmmoTimer.StartRandom(5.0f, 20.0f);
    m_pickupLongJumpModuleTimer.StartRandom(30.0f, 90.0f);

    return Continue();
}

TaskResult<CHL1MPBot> CHL1MPBotScenarioTask::OnTaskUpdate(CHL1MPBot* bot)
{
    if (m_pickupLongJumpModuleTimer.IsElapsed())
    {
        if (bot->HasLongJumpModule())
        {
            m_pickupLongJumpModuleTimer.Start(999999.0f);
            return Continue();
        }

        m_pickupLongJumpModuleTimer.StartRandom(20.0f, 60.0f);
        NBotSharedCollectItemTask::ItemCollectFilter<CHL1MPBot, CNavArea> filter{ bot };
        CBaseEntity* longjump = nullptr;

        if (CBotSharedCollectItemsTask<CHL1MPBot, CHL1MPBotPathCost>::IsPossible(bot, &filter, "item_longjump", &longjump))
        {
            auto task = new CBotSharedCollectItemsTask<CHL1MPBot, CHL1MPBotPathCost>(bot, longjump, NBotSharedCollectItemTask::COLLECT_WALK_OVER);
            auto func = std::bind(PickupLongJumpValidatorFunc, std::placeholders::_1, std::placeholders::_2);
            task->SetValidationFunction(func);
            return PauseFor(task, "Picking up a long jump module!");
        }
    }

    if (m_pickupWeaponsTimer.IsElapsed())
    {
        m_pickupWeaponsTimer.StartRandom(15.0f, 30.0f);
        CHL1MPPickupWeaponFilter filter{ bot };
        CBaseEntity* weapon = nullptr;

        if (CBotSharedCollectItemsTask<CHL1MPBot, CHL1MPBotPathCost>::IsPossible(bot, &filter, "weapon_*", &weapon))
        {
            auto task = new CBotSharedCollectItemsTask<CHL1MPBot, CHL1MPBotPathCost>(bot, weapon, NBotSharedCollectItemTask::COLLECT_WALK_OVER);
            auto func = std::bind(PickupWeaponValidatorFunc, std::placeholders::_1, std::placeholders::_2);
            task->SetValidationFunction(func);
            return PauseFor(task, "Picking up new weapons!");
        }
    }

    if (m_pickupAmmoTimer.IsElapsed())
    {
        m_pickupAmmoTimer.StartRandom(10.0f, 25.0f);
        const CBotWeapon* weapon = bot->GetInventoryInterface()->GetBestLowAmmoWeapon();

        if (weapon && !weapon->GetWeaponInfo()->GetAmmoSourceEntityClassname().empty())
        {
            NBotSharedCollectItemTask::ItemCollectFilter<CHL1MPBot, CNavArea> filter{ bot };
            CBaseEntity* item = nullptr;

            if (CBotSharedCollectItemsTask<CHL1MPBot, CHL1MPBotPathCost>::IsPossible(bot, &filter, weapon->GetWeaponInfo()->GetAmmoSourceEntityClassname().c_str(), &item))
            {
                auto task = new CBotSharedCollectItemsTask<CHL1MPBot, CHL1MPBotPathCost>(bot, item, NBotSharedCollectItemTask::COLLECT_WALK_OVER);
                auto func = std::bind(NBotSharedCollectItemTask::ValidatorIsVisible<CHL1MPBot>, std::placeholders::_1, std::placeholders::_2);
                task->SetValidationFunction(func);
                return PauseFor(task, "Picking up some ammo!");
            }
        }
    }

    return Continue();
}

QueryAnswerType CHL1MPBotScenarioTask::ShouldPickup(CBaseBot* me, CBaseEntity* item)
{
    const char* classname = gamehelpers->GetEntityClassname(item);

    if (strncasecmp("weapon_", classname, 7) == 0)
    {
        if (me->GetInventoryInterface()->FindWeaponByClassname(classname) != nullptr)
        {
            // already owns this weapon
            return ANSWER_NO;
        }
    }

    return ANSWER_UNDEFINED;
}
