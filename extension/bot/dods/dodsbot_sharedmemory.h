#ifndef __NAVBOT_DODSBOT_SHARED_MEMORY_INTERFACE_H_
#define __NAVBOT_DODSBOT_SHARED_MEMORY_INTERFACE_H_

#include <bot/interfaces/sharedmemory.h>

class CDoDSBot;

class CDoDSSharedBotMemory : public ISharedBotMemory
{
public:
	CDoDSSharedBotMemory();

	void Reset() override;
	void OnRoundRestart() override;

	void IncrementDefusersCount(int entindex);
	void DecrementDefusersCount(int entindex);
	int GetDefusersCount(int entindex) const;

	class NotifyDefusing
	{
	public:
		NotifyDefusing() :
			entindex(0), botmem(nullptr)
		{
		}

		NotifyDefusing(CDoDSBot* bot, int entindex);

		~NotifyDefusing()
		{
			if (botmem)
			{
				botmem->DecrementDefusersCount(entindex);
			}
		}

		void Notify(CDoDSBot* bot, int entindex);

	private:
		int entindex;
		CDoDSSharedBotMemory* botmem;
	};

private:
	std::unordered_map<int, int> m_bomb_defusers;
};


#endif // !__NAVBOT_DODSBOT_SHARED_MEMORY_INTERFACE_H_
