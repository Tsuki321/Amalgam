#include "Killstreak.h"

int CKillstreak::GetCurrentStreak()
{
	return m_iCurrentKillstreak;
}

void CKillstreak::ApplyKillstreak(int iLocalIdx)
{
	if (const auto& pLocal = H::Entities.GetLocal())
	{
		if (const auto& pResource = H::Entities.GetResource())
		{
			int iCurrentStreak = GetCurrentStreak();
			// m_iStreaks is a 2D array: [playerIndex * kTFStreak_COUNT + streakType]
			pResource->m_iStreaks(iLocalIdx * kTFStreak_COUNT + kTFStreak_Kills) = iCurrentStreak;
			pResource->m_iStreaks(iLocalIdx * kTFStreak_COUNT + kTFStreak_KillsAll) = iCurrentStreak;
			pResource->m_iStreaks(iLocalIdx * kTFStreak_COUNT + kTFStreak_Ducks) = iCurrentStreak;
			pResource->m_iStreaks(iLocalIdx * kTFStreak_COUNT + kTFStreak_Duck_levelup) = iCurrentStreak;

			// m_nStreaks is a void* pointer to an array of 4 ints (one for each streak type)
			int* pStreaks = reinterpret_cast<int*>(pLocal->m_nStreaks());
			if (pStreaks)
			{
				pStreaks[kTFStreak_Kills] = iCurrentStreak;
				pStreaks[kTFStreak_KillsAll] = iCurrentStreak;
				pStreaks[kTFStreak_Ducks] = iCurrentStreak;
				pStreaks[kTFStreak_Duck_levelup] = iCurrentStreak;
			}
		}
	}
}

void CKillstreak::PlayerDeath(IGameEvent* pEvent)
{
	const int attacker = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
	const int userid = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));

	int iLocalPlayerIdx = I::EngineClient->GetLocalPlayer();
	if (userid == iLocalPlayerIdx)
	{
		Reset();
		return;
	}

	auto pLocal = H::Entities.GetLocal();
	if (attacker != iLocalPlayerIdx ||
		attacker == userid ||
		!pLocal)
		return;

	if (!pLocal->IsAlive())
	{
		if (m_iCurrentKillstreak)
			Reset();
		return;
	}
	const auto iWeaponID = pEvent->GetInt("weaponid");

	m_iCurrentKillstreak++;
	m_mKillstreakMap[iWeaponID]++;

	pEvent->SetInt("kill_streak_total", GetCurrentStreak());
	pEvent->SetInt("kill_streak_wep", m_mKillstreakMap[iWeaponID]);

	ApplyKillstreak(iLocalPlayerIdx);
}

void CKillstreak::PlayerSpawn(IGameEvent* pEvent)
{
	if (!Vars::Visuals::Other::KillstreakWeapons.Value)
		return;

	const int userid = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));
	int iLocalPlayerIdx = I::EngineClient->GetLocalPlayer();
	if (userid == iLocalPlayerIdx)
		Reset();

	ApplyKillstreak(iLocalPlayerIdx);
}

void CKillstreak::Reset()
{
	m_iCurrentKillstreak = 0;
	m_mKillstreakMap.clear();
}

