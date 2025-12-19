#ifndef __NAV_PATH_COST_MODIFIER_H_
#define __NAV_PATH_COST_MODIFIER_H_

/*
* Interface for cost modifiers. These allows nav areas to have dynamic path costs.
*/

class CBaseBot;
class CNavArea;
struct Extent;

/**
 * @brief The path cost mod interface
 */
class INavPathCostMod
{
public:
	// How frequently in seconds OnUpdate is called.
	static constexpr float UPDATE_RATE = 2.0f;

	/**
	 * @brief Utility class notify all nav areas of a cost modifier destruction.
	 */
	class NotifyPathCostModDestruction
	{
	public:
		NotifyPathCostModDestruction(const INavPathCostMod* pcm)
		{
			this->pathcostmod = pcm;
		}

		bool operator()(CNavArea* area);

		const INavPathCostMod* pathcostmod;
	};

	/**
	 * @brief Utility class for registing a nav cost mod for all areas within an Extent.
	 */
	class RegisterPathCostModExtent
	{
	public:
		RegisterPathCostModExtent(const INavPathCostMod* pathcostmod, const Extent& extent);

		std::vector<CNavArea*> areas;
	};

	virtual ~INavPathCostMod() = default;
	/**
	 * @brief Called to check if this cost mod is still valid. If invalid, it will be destroyed.
	 * @return True if valid, false if invalid.
	 */
	virtual bool IsValid() const = 0;
	/**
	 * @brief Is this path cost modifier enabled.
	 * @return True if enabled, false if disabled.
	 */
	virtual bool IsEnabled() const = 0;
	/**
	 * @brief Checks if this path cost modifier should be applied to given team index and bot.
	 * @param teamNum Team number to check.
	 * @param bot Bot to check (this is optional and can be NULL).
	 * @return True if the cost modifier should be applied, false otherwise.
	 */
	virtual bool AppliesTo(const int teamNum, CBaseBot* bot = nullptr) const = 0;
	/**
	 * @brief Called to get the modified path cost.
	 * @param area Nav area whose cost is getting modified.
	 * @param originalCost The original cost of the path.
	 * @param cost The current path cost, this changes as it gets modified by other path costs modifiers.
	 * @param teamNum Team number.
	 * @param bot Bot traversing this path (optional, can be NULL if not computing a path for a bot).
	 * @return Modified cost value.
	 */
	virtual float GetCostMod(const CNavArea* area, const float originalCost, const float cost, const int teamNum, CBaseBot* bot = nullptr) const = 0;
	/**
	 * @brief Called by the nav mesh to update the status of this nav path cost modifier.
	 */
	virtual void OnUpdate() = 0;
};

/**
 * @brief Base class for cost modifiers. Destructor notifies nav areas of it's destruction. Does nothing otherwise, override functions as needed.
 */
class CNavPathCostMod : public INavPathCostMod
{
public:
	~CNavPathCostMod() override;

	// Inherited via INavPathCostMod
	bool IsValid() const override { return true; }
	bool IsEnabled() const override { return true; }
	bool AppliesTo(const int teamNum, CBaseBot* bot) const override { return true; }
	float GetCostMod(const CNavArea* area, const float originalCost, const float cost, const int teamNum, CBaseBot* bot = nullptr) const override { return originalCost; }
	void OnUpdate() override {}
};

#endif // !__NAV_PATH_COST_MODIFIER_H_
