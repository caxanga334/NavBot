#ifndef NAVBOT_TF2_MOD_CLASS_SELECTION_H_
#define NAVBOT_TF2_MOD_CLASS_SELECTION_H_
#pragma once

#include <ITextParsers.h>
#include "teamfortress2_shareddefs.h"

class CTF2ClassSelection : public ITextListener_SMC
{
public:
	CTF2ClassSelection();
	~CTF2ClassSelection();

	enum class ClassRosterType
	{
		ROSTER_DEFAULT = 0,
		ROSTER_ATTACKERS,
		ROSTER_DEFENDERS,
		ROSTER_MANNVSMACHINE,

		MAX_ROSTER_TYPES
	};

	struct ClassData // individual class data
	{
		ClassData();

		inline void Init()
		{
			minimum = 0;
			maximum = 0;
			rate = 0;
			teamsize = 0;
		}

		inline bool HasMinimum() const { return minimum > 0; }
		inline bool HasMaximum() const { return maximum > 0; }
		inline bool HasRate() const { return rate > 0; }
		inline bool IsDisabled() const { return maximum < 0; }
		inline int GetMinimum() const { return minimum; }
		inline int GetMaximum() const { return maximum; }
		inline int GetRate() const { return rate; }
		inline int GetTeamSize() const { return teamsize; }
		inline int GetMinimumForRate(const int onteam) const { return onteam / rate; }

		int minimum; // absolute minimum
		int maximum; // absolute maximum
		int rate; // 1 per N players rate
		int teamsize; // minimum team size to allow selection of this class
	};

	struct ParserData
	{
		inline void BeginParse()
		{
			current_data.Init();
			current_roster = ClassRosterType::MAX_ROSTER_TYPES;
			current_class = TeamFortress2::TFClassType::TFClass_Unknown;
			depth = 0;
		}

		ClassData current_data;
		ClassRosterType current_roster;
		TeamFortress2::TFClassType current_class;
		int depth; // section depth
	};

	void LoadClassSelectionData();

	TeamFortress2::TFClassType SelectAClass(TeamFortress2::TFTeam team, ClassRosterType roster) const;
	bool IsClassAboveLimit(TeamFortress2::TFClassType tfclass, TeamFortress2::TFTeam team, ClassRosterType roster) const;

	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	virtual SMCResult ReadSMC_NewSection(const SMCStates* states, const char* name) override;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	virtual SMCResult ReadSMC_KeyValue(const SMCStates* states, const char* key, const char* value) override;

	/**
	 * @brief Called when leaving the current section.
	 *
	 * @param states		Parsing states.
	 * @return				SMCResult directive.
	 */
	virtual SMCResult ReadSMC_LeavingSection(const SMCStates* states) override;

private:
	// class data for all classes, there are 9 classes but the first valid class starts at 1, remember to subtract to avoid OOB access
	ClassData m_classdata[static_cast<int>(ClassRosterType::MAX_ROSTER_TYPES)][TeamFortress2::TFClassType::TFClass_Engineer];
	ParserData m_parserdata;

	void StoreClassData(ClassRosterType roster, TeamFortress2::TFClassType classtype, const ClassData& data);
};

#endif // !NAVBOT_TF2_MOD_CLASS_SELECTION_H_