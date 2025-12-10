#include "AutoDisguise.h"

void CAutoDisguise::Event(IGameEvent* pEvent, uint32_t uHash, CTFPlayer* pLocal)
{
	// Only process player_death events if enabled
	if (!Vars::Misc::AutoDisguise::Enabled.Value)
		return;

	if (uHash != FNV1A::Hash32Const("player_death"))
		return;

	if (!pLocal || !pLocal->IsAlive())
		return;

	// Check if local player is Spy
	if (pLocal->m_iClass() != TF_CLASS_SPY)
		return;

	// Get the attacker (should be local player)
	int iAttacker = pEvent->GetInt("attacker");
	if (iAttacker != I::EngineClient->GetLocalPlayer())
		return;

	// Get the victim
	int iVictim = pEvent->GetInt("userid");
	if (iVictim <= 0 || iVictim > MAX_PLAYERS)
		return;

	// Get victim's player entity
	CTFPlayer* pVictim = I::ClientEntityList->GetClientEntity(iVictim)->As<CTFPlayer>();
	if (!pVictim)
		return;

	// Get the weapon ID from the active weapon check
	auto pWeapon = pLocal->m_hActiveWeapon().Get()->As<CTFWeaponBase>();
	if (!pWeapon)
		return;

	// Check if weapon is Spy knife or revolver
	int iWeaponID = pWeapon->GetWeaponID();
	bool bIsKnife = (iWeaponID == TF_WEAPON_KNIFE);
	bool bIsRevolver = (iWeaponID == TF_WEAPON_REVOLVER);

	// Support for other Spy revolvers
	if (!bIsRevolver)
	{
		switch (iWeaponID)
		{
		case 61: // Ambassador
		case 224: // L'Etranger
		case 460: // Enforcer
		case 525: // Diamondback
		case 161: // Big Kill
			bIsRevolver = true;
			break;
		}
	}

	// Support for other Spy knives
	if (!bIsKnife)
	{
		switch (iWeaponID)
		{
		case 225: // Your Eternal Reward
		case 356: // Conniver's Kunai
		case 461: // Big Earner
		case 574: // Wanga Prick
		case 638: // Sharp Dresser
		case 649: // Spy-cicle
		case 727: // Black Rose
			bIsKnife = true;
			break;
		}
	}

	// Only proceed if kill was with knife or revolver
	if (!bIsKnife && !bIsRevolver)
		return;

	// Get victim's class and team
	int iVictimClass = pVictim->m_iClass();
	int iVictimTeam = pVictim->m_iTeamNum();

	// Only disguise as victim's class if they're on opposite team
	if (pVictim->m_iTeamNum() == pLocal->m_iTeamNum())
		return;

	// Send disguise command: "disguise <class> <team>"
	// Team: -1 for enemy team (auto-detect), 1 for RED, 2 for BLU
	// Class: victim's class number (1-9)
	std::string sCmd = std::format("disguise {} -1", iVictimClass);
	
	// Execute the disguise command silently
	I::EngineClient->ServerCmd(sCmd.c_str(), true);

	// Optional: Add a small delay or check to ensure disguise succeeds
	// Could add logging here for debugging: SDK::Output("AutoDisguise", std::format("Disguised as {} ({})", iVictimClass, iVictimTeam == TF_TEAM_RED ? "RED" : "BLU").c_str());
}