#ifndef NAVBOT_PLUGINBOT_MOVEMENT_INTERFACE_H_
#define NAVBOT_PLUGINBOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/movement.h>

// TO-DO:
// - Implement plugin callbacks for the movement interface
// - Allow plugins to override movement parameters (jump height, step height, etc...)

class CPluginBot;

class CPluginBotMovement : public IMovement
{
public:
	CPluginBotMovement(CBaseBot* bot);
	~CPluginBotMovement() override;


	// let base handle these until the callbacks are implemented.
	//void Jump() override {}
	//void CrouchJump() override {}
	//void DoubleJump() override {}
	//void JumpAcrossGap(const Vector& landing, const Vector& forward) override {}
	//bool ClimbUpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle) override { return false; }
	//void BlastJumpTo(const Vector& start, const Vector& landingGoal, const Vector& forward) override {}
	//bool DoubleJumpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle) override { return false; }

};

#endif // !NAVBOT_PLUGINBOT_MOVEMENT_INTERFACE_H_
