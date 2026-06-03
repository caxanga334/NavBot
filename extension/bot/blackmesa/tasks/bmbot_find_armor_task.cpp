#include NAVBOT_PCH_FILE
#include <mods/blackmesa/blackmesadm_mod.h>
#include <mods/blackmesa/nav/bm_nav_mesh.h>
#include <bot/blackmesa/bmbot.h>
#include <bot/bot_shared_utils.h>
#include "bmbot_find_armor_task.h"

class CBlackMesaArmorCollector final : public botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea>
{
public:
	CBlackMesaArmorCollector(CBlackMesaBot* bot) :
		botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea>(bot, CBlackMesaDeathmatchMod::GetBMMod()->GetModSettings()->GetCollectItemMaxDistance())
	{
		SetCheckCanPickup(true); // query the behavior to see if we should pick up items
		AddSearchPattern("item_battery");
		AddSearchPattern("item_suitcharger");
	}

private:
	bool IsEntityValid(CBaseEntity* entity, CNavArea* area) final
	{
		if (botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea>::IsEntityValid(entity, area))
		{
			if (UtilHelpers::FClassnameIs(entity, "item_suitcharger"))
			{
				bool chargerIsOn = false;
				entprops->GetEntPropBool(entity, Prop_Send, "m_bOn", chargerIsOn);

				// skip empty chargers
				if (!chargerIsOn)
				{
					return false;
				}
			}

			return true;
		}

		return false;
	}
};

bool CBlackMesaBotFindArmorTask::IsPossible(CBlackMesaBot* bot, CBaseEntity** item)
{
	if (bot->GetArmorPercentage() < 0.99f)
	{
		return true;
	}

	CBlackMesaArmorCollector collector(bot);
	collector.DoSearch();
	CBaseEntity* entity = nullptr;

	// randomize a bit
	if (CBaseBot::s_botrng.GetRandomChance(33))
	{
		entity = collector.SelectFarthest();
	}
	else
	{
		entity = collector.SelectNearest();
	}

	if (!entity)
	{
		return false;
	}

	*item = entity;
	return true;
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindArmorTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	CBaseEntity* armorSource = m_armorSource.Get();

	if (!armorSource)
	{
		return Done("Armor source is NULL!");
	}

	m_goal = UtilHelpers::getWorldSpaceCenter(armorSource);
	float range = bot->GetRangeTo(m_goal);
	float time = range / 90.0f;
	m_isCharger = UtilHelpers::FClassnameIs(armorSource, "item_suitcharger");
	m_timeout.Start(time + 10.0f);

	if (!IsArmorSourceValid())
	{
		return Done("Armor source is invalid!");
	}

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindArmorTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (bot->GetArmorPercentage() > 0.99f)
	{
		return Done("I'm at full armor!");
	}

	if (m_timeout.IsElapsed())
	{
		return Done("Timeout timer expired!");
	}

	if (!IsArmorSourceValid())
	{
		return Done("Armor source is invalid!");
	}

	float moveToRange = m_isCharger ? 70.0f : 24.0f;

	if (bot->GetRangeTo(m_goal) > moveToRange)
	{
		if (m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();

			CBlackMesaBotPathCost cost(bot);

			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				return Done("No route to armor source!");
			}
		}

		m_nav.Update(bot);
	}

	if (m_isCharger && bot->GetRangeTo(m_goal) < CBaseExtPlayer::PLAYER_USE_RADIUS)
	{
		bot->GetControlInterface()->AimAt(m_goal, IPlayerController::LOOK_USE, 0.5f, "Looking at armor charger to use it!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetControlInterface()->PressUseButton();
		}
	}

	return Continue();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotFindArmorTask::OnMoveToSuccess(CBlackMesaBot* bot, CPath* path)
{
	m_timeout.StartRandom(10.0f, 15.0f);

	return TryContinue();
}

bool CBlackMesaBotFindArmorTask::IsArmorSourceValid()
{
	CBaseEntity* source = m_armorSource.Get();

	if (source == nullptr)
	{
		return false;
	}

	int effects = 0;
	entprops->GetEntProp(m_armorSource.GetEntryIndex(), Prop_Send, "m_fEffects", effects);

	if ((effects & EF_NODRAW) != 0)
	{
		return false;
	}

	if (m_isCharger)
	{
		bool on = false;
		entprops->GetEntPropBool(m_armorSource.GetEntryIndex(), Prop_Send, "m_bOn", on);

		if (!on)
		{
			return false;
		}
	}

	return true;
}

bool CBlackMesaBotFindArmorTask::FindArmorSource(CBlackMesaBot* bot, CBaseEntity** armorSource, const float maxRange, const bool filterByDistance)
{
	// vector of armor sources
	std::vector<CBaseEntity*> sources;
	Vector start = bot->GetAbsOrigin();

	auto chargerfunc = [&sources, &start, &maxRange](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			float charge = -1.0f;
			entprops->GetEntPropFloat(index, Prop_Send, "m_flCharge", charge);

			const Vector& end = UtilHelpers::getEntityOrigin(entity);
			float range = (start - end).Length();

			if (charge > 0.01f && range <= maxRange)
			{
				sources.push_back(entity);
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_suitcharger", chargerfunc);

	auto batfunc = [&sources, &start, &maxRange](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			const Vector& end = UtilHelpers::getEntityOrigin(entity);
			float range = (start - end).Length();
			int effects = 0;
			entprops->GetEntProp(index, Prop_Send, "m_fEffects", effects);

			if ((effects & EF_NODRAW) != 0)
			{
				return true; // continue
			}

			if (range <= maxRange)
			{
				sources.push_back(entity);
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_battery", batfunc);

	if (sources.empty())
	{
		return false;
	}

	if (sources.size() == 1)
	{
		*armorSource = sources[0];
		return true;
	}

	// select the nearest armor source
	if (filterByDistance)
	{
		float best = maxRange;
		CBaseEntity* out = nullptr;

		for (CBaseEntity* entity : sources)
		{
			const Vector& end = UtilHelpers::getEntityOrigin(entity);
			float range = (start - end).Length();

			if (range < best)
			{
				best = range;
				out = entity;
			}
		}

		if (out == nullptr)
		{
			// this shouldn't happen
			return false;
		}

		*armorSource = out;
	}
	else
	{
		// select a random armor source
		*armorSource = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(sources);
	}

	return true;
}

