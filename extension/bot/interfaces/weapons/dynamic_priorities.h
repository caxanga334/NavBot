#ifndef __NAVBOT_INTERFACE_WEAPONS_DYNAMIC_PRIORITIES_H_
#define __NAVBOT_INTERFACE_WEAPONS_DYNAMIC_PRIORITIES_H_

#include <type_traits>

class CBaseBot;
class CKnownEntity;
class CBotWeapon;

class IDynamicWeaponPriority
{
public:
	
	virtual ~IDynamicWeaponPriority() {}
	/**
	 * @brief Called to configure this dynamic priority instance.
	 * @param args String arguments read from the config file.
	 * @return True if configuration was successful, false if not.
	 */
	virtual bool Configure(const std::vector<std::string>& args) = 0;
	/**
	 * @brief Called to get the dynamic priority value.
	 * @param bot Bot using the weapon.
	 * @param weapon Weapon being used.
	 * @param threat Threat the bot is attacking using this weapon.
	 * @return Weapon dynamic priority value. 0 for no change, positive increases the weapon's priority and negative decreases it.
	 */
	virtual int GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const = 0;

};

class CBaseDynamicPriority : public IDynamicWeaponPriority
{
public:
	CBaseDynamicPriority() :
		m_priority(0)
	{
	}

	// Inherited via IDynamicWeaponPriority
	bool Configure(const std::vector<std::string>& args) override;
	int GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const override { return m_priority; }

protected:
	void SetPriority(int value) { m_priority = value; }
	int GetPriority() const { return m_priority; }

private:
	int m_priority; // priority value 
};

template <typename ValueType>
class CDynamicPriotityWithThreshold : public CBaseDynamicPriority
{
public:
	CDynamicPriotityWithThreshold()
	{
		m_threshold = static_cast<ValueType>(0);
		m_is_greater = false;
	}

	bool Configure(const std::vector<std::string>& args) override
	{
		// this needs at least 3 args.
		if (args.size() < 3) { return false; }

		// run base first
		if (!CBaseDynamicPriority::Configure(args)) { return false; }
 
		if constexpr (std::is_integral_v<ValueType>)
		{
			if constexpr (sizeof(ValueType) > 4U)
			{
				try
				{
					std::int64_t value = std::stoll(args[1]);
					SetThreshold(static_cast<ValueType>(value));
				}
				catch (...)
				{
					return false;
				}
			}
			else
			{
				try
				{
					int value = std::stoi(args[1]);
					SetThreshold(static_cast<ValueType>(value));
				}
				catch (...)
				{
					return false;
				}
			}
		}
		else if constexpr (std::is_floating_point_v<ValueType>)
		{
			try
			{
				float value = std::stof(args[1]);
				SetThreshold(static_cast<ValueType>(value));
			}
			catch (...)
			{
				return false;
			}
		}
		else
		{
			// unhandled type
			return false;
		}

		m_is_greater = UtilHelpers::StringToBoolean(args[2].c_str());
		return true;
	}

protected:
	void SetThreshold(ValueType value) { m_threshold = value; }
	ValueType GetThreshold() const { return m_threshold; }
	void SetIsGreaterComparison(bool value) { m_is_greater = value; }
	bool GetIsGreaterComparison() const { return m_is_greater; }

	int CalculatePriority(ValueType value) const
	{
		if (GetIsGreaterComparison())
		{
			if (value > GetThreshold())
			{
				return GetPriority();
			}
		}
		else
		{
			if (value < GetThreshold())
			{
				return GetPriority();
			}
		}

		return 0;
	}

private:
	ValueType m_threshold;
	bool m_is_greater;
};

class CDynamicPriorityHealth final : public CDynamicPriotityWithThreshold<float>
{
public:
	int GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const final;

};

class CDynamicPriorityEnemyRange final : public CDynamicPriotityWithThreshold<float>
{
public:
	int GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const final;

};

class CDynamicPriorityHasSecondaryAmmo final : public CBaseDynamicPriority
{
public:
	int GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const final;

};

class CDynamicPriotityBotAggression final : public CDynamicPriotityWithThreshold<int>
{
public:
	int GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const final;
};

#endif // !__NAVBOT_INTERFACE_WEAPONS_DYNAMIC_PRIORITIES_H_
