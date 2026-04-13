#ifndef __NAVBOT_HL1MPBOT_USE_CHARGER_TASK_H_
#define __NAVBOT_HL1MPBOT_USE_CHARGER_TASK_H_

class CHL1MPBotUseChargerTask : public AITask<CHL1MPBot>
{
public:
	enum class ChargerType : int
	{
		HEALTH_CHARGER = 0,
		ARMOR_CHARGER,

		MAX_CHARGER_TYPES
	};

	static bool IsPossible(CHL1MPBot* bot, ChargerType type, CBaseEntity** entity);

	CHL1MPBotUseChargerTask(CBaseEntity* entity, ChargerType type) :
		m_entity(entity), m_type(type)
	{
		using namespace std::literals::string_view_literals;

		m_type = type;

		if (type == ChargerType::HEALTH_CHARGER)
		{
			m_name.assign("UseHealthCharger"sv);
		}
		else
		{
			m_name.assign("UseArmorCharger"sv);
		}
	}

	TaskResult<CHL1MPBot> OnTaskStart(CHL1MPBot* bot, AITask<CHL1MPBot>* pastTask) override;
	TaskResult<CHL1MPBot> OnTaskUpdate(CHL1MPBot* bot) override;

	TaskEventResponseResult<CHL1MPBot> OnStuck(CHL1MPBot* bot) override;
	TaskEventResponseResult<CHL1MPBot> OnMoveToFailure(CHL1MPBot* bot, CPath* path, IEventListener::MovementFailureType reason) override;


	const char* GetName() const override { return m_name.c_str(); }
private:
	std::string m_name;
	CMeshNavigator m_nav;
	CHandle<CBaseEntity> m_entity;
	ChargerType m_type;

	bool IsChargerValid() const;
};



#endif // !__NAVBOT_HL1MPBOT_USE_CHARGER_TASK_H_
