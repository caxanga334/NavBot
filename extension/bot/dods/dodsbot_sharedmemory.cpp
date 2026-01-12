#include NAVBOT_PCH_FILE
#include "dodsbot.h"
#include "dodsbot_sharedmemory.h"

CDoDSSharedBotMemory::CDoDSSharedBotMemory()
{
}

void CDoDSSharedBotMemory::Reset()
{
	ISharedBotMemory::Reset();

	m_bomb_defusers.clear();
}

void CDoDSSharedBotMemory::OnRoundRestart()
{
	ISharedBotMemory::OnRoundRestart();
}

void CDoDSSharedBotMemory::IncrementDefusersCount(int entindex)
{
	if (m_bomb_defusers.find(entindex) != m_bomb_defusers.end())
	{
		int& count = m_bomb_defusers[entindex];
		count++;
		return;
	}

	m_bomb_defusers[entindex] = 1;
}

void CDoDSSharedBotMemory::DecrementDefusersCount(int entindex)
{
	if (m_bomb_defusers.find(entindex) != m_bomb_defusers.end())
	{
		int& count = m_bomb_defusers[entindex];
		
		if (--count < 0)
		{
			count = 0;
		}

		return;
	}

	m_bomb_defusers[entindex] = 0;
}

int CDoDSSharedBotMemory::GetDefusersCount(int entindex) const
{
	if (m_bomb_defusers.find(entindex) != m_bomb_defusers.end())
	{
		return m_bomb_defusers.at(entindex);
	}

	return 0;
}

CDoDSSharedBotMemory::NotifyDefusing::NotifyDefusing(CDoDSBot* bot, int entindex)
{
	botmem = bot->GetSharedMemoryInterface();
	botmem->IncrementDefusersCount(entindex);
}

void CDoDSSharedBotMemory::NotifyDefusing::Notify(CDoDSBot* bot, int entindex)
{
	if (!botmem)
	{
		botmem = bot->GetSharedMemoryInterface();
		botmem->IncrementDefusersCount(entindex);
	}
}
