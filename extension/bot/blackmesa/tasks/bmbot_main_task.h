#ifndef NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_

class CBlackMesaBot;

class CBlackMesaBotMainTask : public AITask<CBlackMesaBot>
{
public:
	CBlackMesaBotMainTask();

	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	const char* GetName() const override { return "MainTask"; }
private:


};

#endif // !NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
