#ifndef __NAVBOT_SDKPORT_CONVARREF_EP1_COMPAT_H_
#define __NAVBOT_SDKPORT_CONVARREF_EP1_COMPAT_H_

#if SOURCE_ENGINE == SE_EPISODEONE

class ConVarRef
{
public:
	ConVarRef(const char* name);
	ConVarRef(const char* name, const bool ignoreMissing);

	inline bool IsValid() const { return m_pCvar != nullptr; }

	const char* GetString(void) const;
	float GetFloat(void) const;
	int GetInt(void) const;
	inline bool GetBool() const { return !!GetInt(); }

	void SetValue(const char* pValue);
	void SetValue(float flValue);
	void SetValue(int nValue);
	void SetValue(bool bValue);

private:
	ConVar* m_pCvar;
};



#endif // SOURCE_ENGINE == SE_EPISODEONE

#endif // !__NAVBOT_SDKPORT_CONVARREF_EP1_COMPAT_H_
