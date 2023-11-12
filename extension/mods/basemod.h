#ifndef EXT_BASE_MOD_H_
#define EXT_BASE_MOD_H_

class CBaseExtPlayer;
class CBaseBot;

// Base game mod class
class CBaseMod
{
public:
	CBaseMod();
	virtual ~CBaseMod();

	// Called every server frame
	virtual void Frame() {}

	// Called by the manager when allocating a new player instance
	virtual CBaseExtPlayer* AllocatePlayer(edict_t* edict);
	// Called by the manager when allocating a new bot instance
	virtual CBaseBot* AllocateBot(edict_t* edict);

	virtual const char* GetModName() { return "CBaseMod"; }

private:

};

#endif // !EXT_BASE_MOD_H_
