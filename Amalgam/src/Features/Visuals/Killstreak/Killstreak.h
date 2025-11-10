#pragma once
#include "../../../SDK/SDK.h"

#include <map>

class CKillstreak
{
private:
	int m_iCurrentKillstreak = 0;
	std::map<int, int> m_mKillstreakMap = {};

public:
	int GetCurrentStreak();
	void ApplyKillstreak(int iLocalIdx);
	void PlayerDeath(IGameEvent* pEvent);
	void PlayerSpawn(IGameEvent* pEvent);
	void Reset();
};

ADD_FEATURE(CKillstreak, Killstreak);

