#ifndef __NAVBOT_SDKPORTS_ENTITY_OUTPUT_H_
#define __NAVBOT_SDKPORTS_ENTITY_OUTPUT_H_

#define EVENT_FIRE_ALWAYS	-1

// From game/server/variant_t.h, same on all supported games.
class variant_t
{
public:
	union
	{
		bool bVal;
		string_t iszVal;
		int iVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
	};

	CBaseHandle eVal;
	fieldtype_t fieldType;

	// constructor
	variant_t() : fieldType(FIELD_VOID), iVal(0) {}

	inline bool Bool(void) const { return(fieldType == FIELD_BOOLEAN) ? bVal : false; }
	inline const char* String(void) const { return(fieldType == FIELD_STRING) ? STRING(iszVal) : ToString(); }
	inline string_t StringID(void) const { return(fieldType == FIELD_STRING) ? iszVal : NULL_STRING; }
	inline int Int(void) const { return(fieldType == FIELD_INTEGER) ? iVal : 0; }
	inline float Float(void) const { return(fieldType == FIELD_FLOAT) ? flVal : 0; }
	inline const CHandle<CBaseEntity>& Entity(void) const;
	inline color32 Color32(void) const { return rgbaVal; }
	inline void Vector3D(Vector& vec) const;

	fieldtype_t FieldType(void) { return fieldType; }

protected:

	const char* ToString(void) const;

};

//-----------------------------------------------------------------------------
// Purpose: A COutputEvent consists of an array of these CEventActions. 
//			Each CEventAction holds the information to fire a single input in 
//			a target entity, after a specific delay.
//-----------------------------------------------------------------------------
class CEventAction
{
public:
	// CEventAction(const char* ActionData = NULL);

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none
	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS.

	int m_iIDStamp;	// unique identifier stamp

	// static int s_iNextIDStamp;

	CEventAction* m_pNext;

	// allocates memory from engine.MPool/g_EntityListPool
	/*
	static void* operator new(size_t stAllocateBlock);
	static void* operator new(size_t stAllocateBlock, int nBlockUse, const char* pFileName, int nLine);
	static void operator delete(void* pMem);
	static void operator delete(void* pMem, int nBlockUse, const char* pFileName, int nLine) { operator delete(pMem); }
	*/

	// DECLARE_SIMPLE_DATADESC();
};


//-----------------------------------------------------------------------------
// Purpose: Stores a list of connections to other entities, for data/commands to be
//			communicated along.
//-----------------------------------------------------------------------------
class CBaseEntityOutput
{
public:
	// ~CBaseEntityOutput();

	// void ParseEventAction(const char* EventData);
	// void AddEventAction(CEventAction* pEventAction);
	// void RemoveEventAction(CEventAction* pEventAction);

	// int Save(ISave& save);
	// int Restore(IRestore& restore, int elementCount);

	int NumberOfElements(void)
	{
		int c = 0;

		for (CEventAction* next = GetFirstAction(); next != nullptr; next = next->m_pNext)
		{
			c++;
		}

		return c;
	}

	float GetMaxDelay(void)
	{
		float flMaxDelay = 0;
		CEventAction* ev = m_ActionList;

		while (ev != NULL)
		{
			if (ev->m_flDelay > flMaxDelay)
			{
				flMaxDelay = ev->m_flDelay;
			}
			ev = ev->m_pNext;
		}

		return flMaxDelay;
	}

	fieldtype_t ValueFieldType() { return m_Value.FieldType(); }

	// void FireOutput(variant_t Value, CBaseEntity* pActivator, CBaseEntity* pCaller, float fDelay = 0);

	/// Delete every single action in the action list. 
	// void DeleteAllElements(void);

	CEventAction* GetFirstAction() { return m_ActionList; }

	// const CEventAction* GetActionForTarget(string_t iSearchTarget) const;

	// void ScriptRemoveEventAction(CEventAction* pEventAction, const char* szTarget, const char* szTargetInput, const char* szParameter);
protected:
	variant_t m_Value;
	CEventAction* m_ActionList;
	// DECLARE_SIMPLE_DATADESC();

	// CBaseEntityOutput() {} // this class cannot be created, only it's children

private:
	// CBaseEntityOutput(CBaseEntityOutput&); // protect from accidental copying
};


#endif // !__NAVBOT_SDKPORTS_ENTITY_OUTPUT_H_
