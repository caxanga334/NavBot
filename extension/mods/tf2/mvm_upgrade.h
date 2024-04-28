#ifndef NAVBOT_TF2_MVM_UPGRADE_H_
#define NAVBOT_TF2_MVM_UPGRADE_H_
#pragma once

#include <string>
#include <unordered_set>

struct MvMUpgrade_t
{
	MvMUpgrade_t()
	{
		index = 0;
		attribute.reserve(64);
		increment = 0.0f;
		cap = 0.0f;
		cost = 0;
		quality = 0;
		max_purchases = 0;
	}

	bool operator==(const MvMUpgrade_t& other) const
	{
		return this->index == other.index;
	}

	bool operator!=(const MvMUpgrade_t& other) const
	{
		return this->index != other.index;
	}

	// Max amount of purchases possible
	void FindMaxPurchases()
	{
		if (increment == 0.0f || (increment == cap))
		{
			max_purchases = 1;
			return;
		}

		int count = 0;

		// increment is positive
		if (increment > 0.0f)
		{
			for (float base = 1.0f; base < cap; base += increment)
			{
				++count;
			}
		}
		else if (increment < 0.0f) // increment is negative
		{
			for (float base = 1.0f; base > cap; base += increment)
			{
				++count;
			}
		}

		max_purchases = count;
	}

	const bool CanAfford(const int currency) const
	{
		return currency >= cost;
	}

	int index;
	std::string attribute;
	float increment;
	float cap;
	int cost;
	int quality;
	int max_purchases;
};


// Used by the bot to reference an upgrade bought
struct TF2BotUpgrade_t
{
	TF2BotUpgrade_t(const MvMUpgrade_t* up)
	{
		upgrade = up;
		times_bought = 0;
	}

	const bool IsMaxed() const { return times_bought >= upgrade->max_purchases; }

	const MvMUpgrade_t* upgrade;
	int times_bought;
};


// Upgrade Information
struct TF2BotUpgradeInfo_t
{
	TF2BotUpgradeInfo_t()
	{
		attribute.reserve(64);
		quality = 0; // default
		priority = 1;
		itemslot = 0;
		maxlevel = 0;
		allowedweapons.reserve(4);
		excludedweapons.reserve(4);
		upgrade = nullptr;
	}

	// if allowedweapons is empty, then any weapon is allowed
	const bool AnyWeapon() const { return allowedweapons.empty(); }
	const bool IsWeaponAllowed(const int itemindex) const
	{
		if (AnyWeapon())
			return true;

		return allowedweapons.find(itemindex) != allowedweapons.end();
	}

	const bool AreThereExcludedWeapons() const { return !excludedweapons.empty(); }

	const bool IsWeaponExcluded(const int itemindex) const
	{
		return excludedweapons.find(itemindex) != excludedweapons.end();
	}

	void SetUpgrade(const MvMUpgrade_t* upg)
	{
		this->upgrade = upg;
	}

	int GetUpgradeIndex() const
	{
		return upgrade->index;
	}

	const bool HasLevelLimit() const { return maxlevel > 0; }
	const int GetLevelLimit() const { return maxlevel; }

	std::string attribute;
	int quality;
	int priority;
	int itemslot;
	int maxlevel;
	std::unordered_set<int> allowedweapons;
	std::unordered_set<int> excludedweapons;
	const MvMUpgrade_t* upgrade;
};

#endif // !NAVBOT_TF2_MVM_UPGRADE_H_
