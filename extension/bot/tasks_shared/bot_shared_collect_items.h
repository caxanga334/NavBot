#ifndef __NAVBOT_BOT_SHARED_COLLECT_ITEMS_TASK_H_
#define __NAVBOT_BOT_SHARED_COLLECT_ITEMS_TASK_H_

#include <vector>
#include <string>
#include <functional>
#include <iterator>
#include <limits>
#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/basemod.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>

namespace NBotSharedCollectItemTask
{
	inline constexpr int COLLECT_WALK_OVER = 0; // collect item by walking over it
	inline constexpr int COLLECT_PRESS_USE = 1; // collect item by aiming and pressing the use key on it

	// Filter used to select collectable items
	template <typename BotClass, typename NavAreaClass>
	class ItemCollectFilter : public UtilHelpers::IGenericFilter<CBaseEntity*>
	{
	public:
		ItemCollectFilter(BotClass* bot) :
			m_position(0.0f, 0.0f, 0.0f)
		{
			m_bot = bot;
			m_navarea = nullptr;
		}

		bool IsSelected(CBaseEntity* object) override
		{
			m_position = UtilHelpers::getWorldSpaceCenter(object);
			m_navarea = static_cast<NavAreaClass*>(TheNavMesh->GetNearestNavArea(m_position, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 2.0f));

			if (!IsItemValid(object, m_navarea))
			{
				return false;
			}

			if (!CanPickup(object, m_bot, m_navarea))
			{
				return false;
			}

			return true;
		}

		/**
		 * @brief Called to determine if the given bot is able to pick up the item.
		 * @param object Item to pick up.
		 * @param bot Bot that will be picking up the item.
		 * @param navarea Nearest nav area of the item. May be NULL.
		 * @return True to pick up the item, false to ignore it.
		 */
		virtual bool CanPickup(CBaseEntity* object, BotClass* bot, NavAreaClass* navarea)
		{
			// Behavior tells me I can't pick up it
			if (bot->GetBehaviorInterface()->ShouldPickup(static_cast<CBaseBot*>(bot), object) == ANSWER_NO)
			{
				return false;
			}

			if (navarea)
			{
				// bot can't reach it
				if (!bot->GetMovementInterface()->IsAreaTraversable(navarea))
				{
					return false;
				}
			}

			return true;
		}
		/**
		 * @brief Called to determine if the given item is valid.
		 * @param object Item being checked.
		 * @param navarea Nearest nav area of the item. May be NULL.
		 * @return True if valid, false if invalid.
		 */
		virtual bool IsItemValid(CBaseEntity* object, NavAreaClass* navarea)
		{
			// Must be in the mesh
			if (navarea == nullptr)
			{
				return false;
			}

			// In multiplayer games, items waiting for respawn generally have nodraw set
			if (entityprops::IsEffectActiveOnEntity(object, EF_NODRAW))
			{
				return false;
			}

			return true;
		}

		/**
		 * @brief Called after the items search.
		 * @param items Vector of items collected. Can be further filtered (IE: distance)
		 */
		virtual void PostSearchFilter(std::vector<CBaseEntity*>& items) {}

		BotClass* GetBot() const { return m_bot; }
		const Vector& GetPosition() const { return m_position; }
		NavAreaClass* GetNearestNavArea() const { return m_navarea; }

	private:
		BotClass* m_bot;
		NavAreaClass* m_navarea;
		Vector m_position;
	};

	// Simple validator function for the CBotSharedCollectItemsTask, checks if the entity has the nodraw flag
	template <typename BotClass>
	inline bool ValidatorIsVisible(BotClass* bot, CBaseEntity* entity)
	{
		if (entityprops::IsEffectActiveOnEntity(entity, EF_NODRAW))
		{
			return false;
		}

		return true;
	}
}

template <typename BT, typename CT>
class CBotSharedCollectItemsTask : public AITask<BT>
{
public:
	/**
	 * @brief Call this to determine if it's possible for the bot to collect items.
	 * @tparam FilterType Filter class, must use the ItemCollectFilter class declared in this file as a base.
	 * @param bot Bot that will be collecting items.
	 * @param filter Item filter.
	 * @param searchPatterns String Vector of entity search pattern. IE: item_healthkit, item_ammo_*
	 * @param output Vector to store the entity pointers of items found.
	 * @param maxSearchDistOverride Optional, if larger than one, overrides the maximum search distance, default depends on the mod settings.
	 * @return True if at least one item was found.
	 */
	template<typename FilterType>
	static bool IsPossible(BT* bot, FilterType* filter, const std::vector<std::string>& searchPatterns, std::vector<CBaseEntity*>& output, const float maxSearchDistOverride = -1.0f)
	{
		const float maxrange = maxSearchDistOverride >= 1.0f ? maxSearchDistOverride : extmanager->GetMod()->GetModSettings()->GetCollectItemMaxDistance();
		botsharedutils::IsReachableAreas collector{ bot, maxrange };
		collector.Execute();

		auto functor = [&filter, &output, &collector](int index, edict_t* edict, CBaseEntity* entity) {
			if (filter->IsSelected(entity))
			{
				if (collector.IsReachable(filter->GetNearestNavArea(), nullptr))
				{
					output.push_back(entity);
				}
			}

			return true;
		};

		for (const std::string& pattern : searchPatterns)
		{
			UtilHelpers::ForEachEntityOfClassname(pattern.c_str(), functor);
		}

		filter->PostSearchFilter(output);

		if (output.empty())
		{
			return false;
		}

		return true;
	}

	// Overload for a single item, selects the nearest item
	template<typename FilterType>
	static bool IsPossible(BT* bot, FilterType* filter, const char* searchPattern, CBaseEntity** output, const float maxSearchDistOverride = -1.0f)
	{
		const float maxrange = maxSearchDistOverride >= 1.0f ? maxSearchDistOverride : extmanager->GetMod()->GetModSettings()->GetCollectItemMaxDistance();
		botsharedutils::IsReachableAreas collector{ bot, maxrange };
		collector.Execute();
		CBaseEntity* best = nullptr;
		float smallest_dist = std::numeric_limits<float>::max();

		auto functor = [&filter, &collector, &best, &smallest_dist](int index, edict_t* edict, CBaseEntity* entity) {
			if (filter->IsSelected(entity))
			{
				float cost = 0.0f;

				if (collector.IsReachable(filter->GetNearestNavArea(), &cost))
				{
					if (cost < smallest_dist)
					{
						smallest_dist = cost;
						best = entity;
					}
				}
			}

			return true;
		};

		UtilHelpers::ForEachEntityOfClassname(searchPattern, functor);

		if (best == nullptr)
		{
			return false;
		}

		*output = best;
		return true;
	}

	CBotSharedCollectItemsTask(BT* bot, const std::vector<CBaseEntity*>& items, int collectmethod) :
		m_pathcost(bot)
	{
		for (CBaseEntity* item : items)
		{
			m_items.emplace_back(item);
		}

		m_collectmethod = collectmethod;
		m_it = m_items.begin();
		m_pathfailures = 0;
	}

	CBotSharedCollectItemsTask(BT* bot, CBaseEntity* item, int collectmethod) :
		m_pathcost(bot)
	{
		m_items.emplace_back(item);
		m_collectmethod = collectmethod;
		m_it = m_items.begin();
		m_pathfailures = 0;
	}

	// Sets the additional validation function to skip collecting an item, otherwise the bot only skips an items when the entity becomes NULL.
	// bool (BT*, CBaseEntity*)
	template <typename F>
	void SetValidationFunction(F func)
	{
		m_validatorFunc = func;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		if (m_items.empty())
		{
			return AITask<BT>::Done("No items to collect!");
		}

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot)
	{
		CHandle<CBaseEntity>& ehandle = *m_it;
		CBaseEntity* item = ehandle.Get();

		if (!item)
		{
			m_it = std::next(m_it);
			m_pathfailures = 0;

			if (m_it == m_items.end())
			{
				return AITask<BT>::Done("Done collecting items.");
			}

			return AITask<BT>::Continue();
		}

		if (m_validatorFunc)
		{
			const bool result = m_validatorFunc(bot, item);

			if (!result)
			{
				m_it = std::next(m_it);
				m_pathfailures = 0;

				if (m_it == m_items.end())
				{
					return AITask<BT>::Done("Done collecting items.");
				}

				return AITask<BT>::Continue();
			}
		}

		if (m_nav.NeedsRepath())
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(item);
			m_nav.StartRepathTimer();

			if (!m_nav.ComputePathToPosition(bot, pos, m_pathcost, 0.0f, true))
			{
				m_nav.ForceRepath();

				if (++m_pathfailures >= 20)
				{
					if (bot->IsDebugging(BOTDEBUG_TASKS))
					{
						bot->DebugPrintToConsole(255, 0, 0, "%s FAILED TO FIND A PATH TO COLLECT ITEM AT %g %g %g \n", bot->GetDebugIdentifier(), pos.x, pos.y, pos.z);
					}

					m_it = std::next(m_it);
					m_pathfailures = 0;

					if (m_it == m_items.end())
					{
						return AITask<BT>::Done("Done collecting items.");
					}

					return AITask<BT>::Continue();
				}
			}
		}

		if (m_collectmethod == NBotSharedCollectItemTask::COLLECT_PRESS_USE)
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(item);

			if ((bot->GetEyeOrigin() - pos).IsLengthLessThan(CBaseExtPlayer::PLAYER_USE_RADIUS))
			{
				//AimAt(const Vector& pos, const LookPriority priority, const float duration, const char* reason = nullptr)
				bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_PRIORITY, 1.0f, "Looking at item to collect it!");

				if (bot->GetControlInterface()->IsAimOnTarget())
				{
					bot->GetControlInterface()->PressUseButton();
				}
			}
			else
			{
				m_nav.Update(bot);
			}
		}
		else
		{
			m_nav.Update(bot);
		}

		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnStuck(BT* bot) override
	{
		return AITask<BT>::TryContinue();
	}

	const char* GetName() const override { return "CollectItems"; }

private:
	CT m_pathcost;
	CMeshNavigator m_nav;
	std::vector<CHandle<CBaseEntity>> m_items;
	int m_collectmethod;
	std::vector<CHandle<CBaseEntity>>::iterator m_it;
	std::function<bool(BT*, CBaseEntity*)> m_validatorFunc;
	int m_pathfailures;
};


#endif // !__NAVBOT_BOT_SHARED_COLLECT_ITEMS_TASK_H_
