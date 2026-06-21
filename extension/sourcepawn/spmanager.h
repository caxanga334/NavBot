#ifndef __NAVBOT_SOURCEPAWN_MANAGER_H_
#define __NAVBOT_SOURCEPAWN_MANAGER_H_

#include <IHandleSys.h>

class SourcePawnManager : public SourceMod::IHandleTypeDispatch
{
public:
	SourcePawnManager();
	virtual ~SourcePawnManager();

	enum HandleTypes
	{
		HANDLE_NAVIGATOR = 0,
		HANDLE_WEAPONPRIOFACTORY,
		HANDLE_WEAPONPRIOINSTANCE,
		HANDLE_NAVCOLLECTOR,
		HANDLE_NAVAREAVECTOR,
		HANDLE_NAVBLOCKER,

		MAX_HANDLE_TYPES
	};

	static void Init();

	// Inherited via IHandleTypeDispatch
	void OnHandleDestroy(SourceMod::HandleType_t type, void* object) override;

	/**
	 * @brief Called to get the size of a handle's memory usage in bytes.
	 * Implementation is optional.
	 *
	 * @param type		Handle type.
	 * @param object	Handle internal object.
	 * @param pSize		Pointer to store the approximate memory usage in bytes.
	 * @return			True on success, false if not implemented.
	 */
	bool GetHandleApproxSize(SourceMod::HandleType_t type, void* object, unsigned int* pSize) override;

	void SetupHandles();

	SourceMod::HandleType_t GetMeshNavigatorHandleType() const { return m_handletypes[HANDLE_NAVIGATOR]; }
	SourceMod::HandleType_t GetWeaponPriorityFactoryHandleType() const { return m_handletypes[HANDLE_WEAPONPRIOFACTORY]; }
	SourceMod::HandleType_t GetWeaponPriorityInstanceHandleType() const { return m_handletypes[HANDLE_WEAPONPRIOINSTANCE]; }
	SourceMod::HandleType_t GetNavCollectorHandleType() const { return m_handletypes[HANDLE_NAVCOLLECTOR]; }
	SourceMod::HandleType_t GetNavAreaVectorHandleType() const { return m_handletypes[HANDLE_NAVAREAVECTOR]; }
	SourceMod::HandleType_t GetNavBlockerHandleType() const { return m_handletypes[HANDLE_NAVBLOCKER]; }
private:
	std::array<SourceMod::HandleType_t, static_cast<std::size_t>(MAX_HANDLE_TYPES)> m_handletypes;

	void SetupNavigatorHandle();
	void SetupWeaponDynPrioFactoryHandle();
	void SetupWeaponDynPrioInstanceHandle();
	void SetupNavCollectorHandle();
	void SetupNavAreaVectorHandle();
	void SetupNavBlockerHandle();
};


// Singleton to access the SourcePawn manager
extern std::unique_ptr<SourcePawnManager> spmanager;

#endif //__NAVBOT_SOURCEPAWN_MANAGER_H_