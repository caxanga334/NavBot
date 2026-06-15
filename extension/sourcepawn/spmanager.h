#ifndef __NAVBOT_SOURCEPAWN_MANAGER_H_
#define __NAVBOT_SOURCEPAWN_MANAGER_H_

#include <IHandleSys.h>

class SourcePawnManager : public SourceMod::IHandleTypeDispatch
{
public:
	SourcePawnManager();
	virtual ~SourcePawnManager();

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

	SourceMod::HandleType_t GetMeshNavigatorHandleType() const { return m_meshnavigator_type; }
	SourceMod::HandleType_t GetWeaponPriorityFactoryHandleType() const { return m_weaponpriofactory_type; }
	SourceMod::HandleType_t GetWeaponPriorityInstanceHandleType() const { return m_weaponprioinstance_type; }
private:
	SourceMod::HandleType_t m_meshnavigator_type; // handle type for CMeshNavigator
	SourceMod::HandleType_t m_weaponpriofactory_type;
	SourceMod::HandleType_t m_weaponprioinstance_type;
};


// Singleton to access the SourcePawn manager
extern std::unique_ptr<SourcePawnManager> spmanager;

#endif //__NAVBOT_SOURCEPAWN_MANAGER_H_