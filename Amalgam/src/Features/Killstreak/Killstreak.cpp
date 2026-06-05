#include "Killstreak.h"

int CKillstreak::GetCurrentStreak()
{
	return m_iCurrentKillstreak;
}

void CKillstreak::ApplyKillstreak(int iLocalIdx)
{
	if (auto pLocal = H::Entities.GetLocal())
	{
		if (auto pResource = H::Entities.GetResource())
		{
			int iCurrentStreak = GetCurrentStreak();
			pResource->SetStreak(iLocalIdx, kTFStreak_Kills, iCurrentStreak);
			pResource->SetStreak(iLocalIdx, kTFStreak_KillsAll, iCurrentStreak);
			pResource->SetStreak(iLocalIdx, kTFStreak_Ducks, iCurrentStreak);
			pResource->SetStreak(iLocalIdx, kTFStreak_Duck_levelup, iCurrentStreak);

			pLocal->m_nStreaks(kTFStreak_Kills) = iCurrentStreak;
			pLocal->m_nStreaks(kTFStreak_KillsAll) = iCurrentStreak;
			pLocal->m_nStreaks(kTFStreak_Ducks) = iCurrentStreak;
			pLocal->m_nStreaks(kTFStreak_Duck_levelup) = iCurrentStreak;
		}
	}
}

void CKillstreak::PlayerDeath(IGameEvent* pEvent)
{
	const int attacker = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
	const int userid = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));

	int iLocalPlayerIdx = H::Entities.GetLocalPlayerIndex();
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

void CKillstreak::PlayerSpawn()
{
	if (!Vars::Visuals::UI::KillstreakWeapons.Value)
		return;

	int iLocalPlayerIdx = H::Entities.GetLocalPlayerIndex();

	Reset();
	ApplyKillstreak(iLocalPlayerIdx);
}

void CKillstreak::Reset()
{
	m_iCurrentKillstreak = 0;
	m_mKillstreakMap.clear();
}
