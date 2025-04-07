#include "Misc.h"

#include "../Backtrack/Backtrack.h"
#include "../TickHandler/TickHandler.h"
#include "../Players/PlayerUtils.h"
#include "../Aimbot/AutoRocketJump/AutoRocketJump.h"

void CMisc::RunPre(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	CheatsBypass();
	PingReducer();
	WeaponSway();

	if (!pLocal)
		return;

	AntiAFK(pLocal, pCmd);
	InstantRespawnMVM(pLocal);
	AutoVaccinator(pLocal, pCmd);

	if (!pLocal->IsAlive() || pLocal->IsAGhost() || pLocal->m_MoveType() != MOVETYPE_WALK || pLocal->IsSwimming() || pLocal->InCond(TF_COND_SHIELD_CHARGE) || pLocal->InCond(TF_COND_HALLOWEEN_KART))
		return;

	AutoJump(pLocal, pCmd);
	EdgeJump(pLocal, pCmd);
	AutoJumpbug(pLocal, pCmd);
	AutoStrafe(pLocal, pCmd);
	AutoPeek(pLocal, pCmd);
	MovementLock(pLocal, pCmd);
	BreakJump(pLocal, pCmd);
}

void CMisc::RunPost(CTFPlayer* pLocal, CUserCmd* pCmd, bool pSendPacket)
{
	if (!pLocal || !pLocal->IsAlive() || pLocal->IsAGhost() || pLocal->m_MoveType() != MOVETYPE_WALK || pLocal->IsSwimming() || pLocal->InCond(TF_COND_SHIELD_CHARGE))
		return;

	EdgeJump(pLocal, pCmd, true);
	TauntKartControl(pLocal, pCmd);
	FastMovement(pLocal, pCmd);
}



void CMisc::AutoJump(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::Bunnyhop.Value)
		return;

	static bool bStaticJump = false, bStaticGrounded = false, bLastAttempted = false;
	const bool bLastJump = bStaticJump, bLastGrounded = bStaticGrounded;
	const bool bCurJump = bStaticJump = pCmd->buttons & IN_JUMP, bCurGrounded = bStaticGrounded = pLocal->m_hGroundEntity();

	if (bCurJump && bLastJump && (bCurGrounded ? !pLocal->IsDucking() : true))
	{
		if (!(bCurGrounded && !bLastGrounded))
			pCmd->buttons &= ~IN_JUMP;

		if (!(pCmd->buttons & IN_JUMP) && bCurGrounded && !bLastAttempted)
			pCmd->buttons |= IN_JUMP;
	}

	if (Vars::Misc::Game::AntiCheatCompatibility.Value)
	{	// prevent more than 9 bhops occurring. if a server has this under that threshold they're retarded anyways
		static int iJumps = 0;
		if (bCurGrounded)
		{
			if (!bLastGrounded && pCmd->buttons & IN_JUMP)
				iJumps++;
			else
				iJumps = 0;

			if (iJumps > 9)
				pCmd->buttons &= ~IN_JUMP;
		}
	}
	bLastAttempted = pCmd->buttons & IN_JUMP;
}

void CMisc::AutoJumpbug(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::AutoJumpbug.Value || !(pCmd->buttons & IN_DUCK) || pLocal->m_hGroundEntity() || pLocal->m_vecVelocity().z > -650.f)
		return;

	float flUnduckHeight = 20 * pLocal->m_flModelScale();
	float flTraceDistance = flUnduckHeight + 2;

	CGameTrace trace = {};
	CTraceFilterWorldAndPropsOnly filter = {};

	Vec3 vOrigin = pLocal->m_vecOrigin();
	SDK::TraceHull(vOrigin, vOrigin - Vec3(0, 0, flTraceDistance), pLocal->m_vecMins(), pLocal->m_vecMaxs(), pLocal->SolidMask(), &filter, &trace);
	if (!trace.DidHit() || trace.fraction * flTraceDistance < flUnduckHeight) // don't try if we aren't in range to unduck or are too low
		return;

	pCmd->buttons &= ~IN_DUCK;
	pCmd->buttons |= IN_JUMP;
}

void CMisc::AutoStrafe(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::AutoStrafe.Value || pLocal->m_hGroundEntity() || !(pLocal->m_afButtonLast() & IN_JUMP) && (pCmd->buttons & IN_JUMP))
		return;

	switch (Vars::Misc::Movement::AutoStrafe.Value)
	{
	case Vars::Misc::Movement::AutoStrafeEnum::Legit:
	{
		static auto cl_sidespeed = U::ConVars.FindVar("cl_sidespeed");
		const float flSideSpeed = cl_sidespeed->GetFloat();

		if (pCmd->mousedx)
		{
			pCmd->forwardmove = 0.f;
			pCmd->sidemove = pCmd->mousedx > 0 ? flSideSpeed : -flSideSpeed;
		}
		break;
	}
	case Vars::Misc::Movement::AutoStrafeEnum::Directional:
	{
		// credits: KGB
		if (!(pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)))
			break;

		float flForward = pCmd->forwardmove, flSide = pCmd->sidemove;

		Vec3 vForward, vRight; Math::AngleVectors(pCmd->viewangles, &vForward, &vRight, nullptr);
		vForward.z = vRight.z = 0.f;
		vForward.Normalize(), vRight.Normalize();

		Vec3 vWishDir = {}; Math::VectorAngles({ vForward.x * flForward + vRight.x * flSide, vForward.y * flForward + vRight.y * flSide, 0.f }, vWishDir);
		Vec3 vCurDir = {}; Math::VectorAngles(pLocal->m_vecVelocity(), vCurDir);
		float flDirDelta = Math::NormalizeAngle(vWishDir.y - vCurDir.y);
		if (fabsf(flDirDelta) > Vars::Misc::Movement::AutoStrafeMaxDelta.Value)
			break;

		float flTurnScale = Math::RemapVal(Vars::Misc::Movement::AutoStrafeTurnScale.Value, 0.f, 1.f, 0.9f, 1.f);
		float flRotation = DEG2RAD((flDirDelta > 0.f ? -90.f : 90.f) + flDirDelta * flTurnScale);
		float flCosRot = cosf(flRotation), flSinRot = sinf(flRotation);

		pCmd->forwardmove = flCosRot * flForward - flSinRot * flSide;
		pCmd->sidemove = flSinRot * flForward + flCosRot * flSide;
	}
	}
}

void CMisc::AutoPeek(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	static bool bReturning = false;
	if (Vars::CL_Move::AutoPeek.Value)
	{
		const Vec3 vLocalPos = pLocal->m_vecOrigin();

		if (G::Attacking && m_bPeekPlaced)
			bReturning = true;
		if (!bReturning && !pLocal->m_hGroundEntity())
			m_bPeekPlaced = false;

		if (!m_bPeekPlaced)
		{
			m_vPeekReturnPos = vLocalPos;
			m_bPeekPlaced = true;
		}
		else
		{
			static Timer tTimer = {};
			if (tTimer.Run(0.7f))
				H::Particles.DispatchParticleEffect("ping_circle", m_vPeekReturnPos, {});
		}

		if (bReturning)
		{
			if (vLocalPos.DistTo(m_vPeekReturnPos) < 8.f)
			{
				bReturning = false;
				return;
			}

			SDK::WalkTo(pCmd, pLocal, m_vPeekReturnPos);
			pCmd->buttons &= ~IN_JUMP;
		}
	}
	else
		m_bPeekPlaced = bReturning = false;
}

void CMisc::MovementLock(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	static bool bLock = false;

	if (!Vars::Misc::Movement::MovementLock.Value)
	{
		bLock = false;
		return;
	}

	static Vec3 vMove = {}, vView = {};
	if (!bLock)
	{
		bLock = true;
		vMove = { pCmd->forwardmove, pCmd->sidemove, pCmd->upmove };
		vView = pCmd->viewangles;
	}

	pCmd->forwardmove = vMove.x, pCmd->sidemove = vMove.y, pCmd->upmove = vMove.z;
	SDK::FixMovement(pCmd, vView, pCmd->viewangles);
}

void CMisc::BreakJump(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::BreakJump.Value || F::AutoRocketJump.IsRunning())
		return;

	static bool bStaticJump = false;
	const bool bLastJump = bStaticJump;
	const bool bCurrJump = bStaticJump = pCmd->buttons & IN_JUMP;

	static int iTickSinceGrounded = -1;
	if (pLocal->m_hGroundEntity().Get())
		iTickSinceGrounded = -1;
	iTickSinceGrounded++;

	switch (iTickSinceGrounded)
	{
	case 0:
		if (bLastJump || !bCurrJump || pLocal->IsDucking())
			return;
		break;
	case 1:
		break;
	default:
		return;
	}

	pCmd->buttons |= IN_DUCK;
}

void CMisc::AntiAFK(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	static Timer tTimer = {};

	static auto mp_idledealmethod = U::ConVars.FindVar("mp_idledealmethod");
	static auto mp_idlemaxtime = U::ConVars.FindVar("mp_idlemaxtime");
	const int iIdleMethod = mp_idledealmethod->GetInt();
	const float flMaxIdleTime = mp_idlemaxtime->GetFloat();

	if (pCmd->buttons & (IN_MOVELEFT | IN_MOVERIGHT | IN_FORWARD | IN_BACK) || !pLocal->IsAlive())
		tTimer.Update();
	else if (Vars::Misc::Automation::AntiAFK.Value && iIdleMethod && tTimer.Check(flMaxIdleTime * 60.f - 10.f)) // trigger 10 seconds before kick
		pCmd->buttons |= I::GlobalVars->tickcount % 2 ? IN_FORWARD : IN_BACK;
}

void CMisc::InstantRespawnMVM(CTFPlayer* pLocal)
{
	if (Vars::Misc::MannVsMachine::InstantRespawn.Value && I::EngineClient->IsInGame() && !pLocal->IsAlive())
	{
		KeyValues* kv = new KeyValues("MVM_Revive_Response");
		kv->SetInt("accepted", 1);
		I::EngineClient->ServerCmdKeyValues(kv);
	}
}

void CMisc::CheatsBypass()
{
	static bool bCheatSet = false;
	static auto sv_cheats = U::ConVars.FindVar("sv_cheats");
	if (Vars::Misc::Exploits::CheatsBypass.Value)
	{
		sv_cheats->m_nValue = 1;
		bCheatSet = true;
	}
	else if (bCheatSet)
	{
		sv_cheats->m_nValue = 0;
		bCheatSet = false;
	}
}

void CMisc::PingReducer()
{
	static Timer tTimer = {};
	if (!tTimer.Run(0.1f))
		return;

	auto pNetChan = reinterpret_cast<CNetChannel*>(I::EngineClient->GetNetChannelInfo());
	if (!pNetChan)
		return;

	static auto cl_cmdrate = U::ConVars.FindVar("cl_cmdrate");
	const int iCmdRate = cl_cmdrate->GetInt();

	// force highest cl_updaterate command possible
	static auto sv_maxupdaterate = U::ConVars.FindVar("sv_maxupdaterate");
	const int iMaxUpdateRate = sv_maxupdaterate->GetInt();

	const int iTarget = Vars::Misc::Exploits::PingReducer.Value ? Vars::Misc::Exploits::PingTarget.Value : iCmdRate;
	if (m_iWishCmdrate != iTarget)
	{
		NET_SetConVar cmd("cl_cmdrate", std::to_string(m_iWishCmdrate = iTarget).c_str());
		pNetChan->SendNetMsg(cmd);
	}

	if (m_iWishUpdaterate != iMaxUpdateRate)
	{
		NET_SetConVar cmd("cl_updaterate", std::to_string(m_iWishUpdaterate = iMaxUpdateRate).c_str());
		pNetChan->SendNetMsg(cmd);
	}
}

void CMisc::WeaponSway()
{
	static auto cl_wpn_sway_interp = U::ConVars.FindVar("cl_wpn_sway_interp");
	static auto cl_wpn_sway_scale = U::ConVars.FindVar("cl_wpn_sway_scale");

	bool bSway = Vars::Visuals::Viewmodel::SwayInterp.Value || Vars::Visuals::Viewmodel::SwayScale.Value;
	cl_wpn_sway_interp->SetValue(bSway ? Vars::Visuals::Viewmodel::SwayInterp.Value : 0.f);
	cl_wpn_sway_scale->SetValue(bSway ? Vars::Visuals::Viewmodel::SwayScale.Value : 0.f);
}



void CMisc::TauntKartControl(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	// Handle Taunt Slide
	if (Vars::Misc::Automation::TauntControl.Value && pLocal->IsTaunting() && pLocal->m_bAllowMoveDuringTaunt())
	{
		if (pLocal->m_bTauntForceMoveForward())
		{
			if (pCmd->buttons & IN_BACK)
				pCmd->viewangles.x = 91.f;
			else if (!(pCmd->buttons & IN_FORWARD))
				pCmd->viewangles.x = 90.f;
		}
		if (pCmd->buttons & IN_MOVELEFT)
			pCmd->sidemove = pCmd->viewangles.x != 90.f ? -50.f : -450.f;
		else if (pCmd->buttons & IN_MOVERIGHT)
			pCmd->sidemove = pCmd->viewangles.x != 90.f ? 50.f : 450.f;

		Vec3 vAngle = I::EngineClient->GetViewAngles();
		pCmd->viewangles.y = vAngle.y;

		G::SilentAngles = true;
	}
	else if (Vars::Misc::Automation::KartControl.Value && pLocal->InCond(TF_COND_HALLOWEEN_KART))
	{
		const bool bForward = pCmd->buttons & IN_FORWARD;
		const bool bBack = pCmd->buttons & IN_BACK;
		const bool bLeft = pCmd->buttons & IN_MOVELEFT;
		const bool bRight = pCmd->buttons & IN_MOVERIGHT;

		const bool flipVar = I::GlobalVars->tickcount % 2;
		if (bForward && (!bLeft && !bRight || !flipVar))
		{
			pCmd->forwardmove = 450.f;
			pCmd->viewangles.x = 0.f;
		}
		else if (bBack && (!bLeft && !bRight || !flipVar))
		{
			pCmd->forwardmove = 450.f;
			pCmd->viewangles.x = 91.f;
		}
		else if (pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT))
		{
			if (flipVar || !F::Ticks.CanChoke())
			{	// you could just do this if you didn't care about viewangles
				const Vec3 vecMove(pCmd->forwardmove, pCmd->sidemove, 0.f);
				const float flLength = vecMove.Length();
				Vec3 angMoveReverse;
				Math::VectorAngles(vecMove * -1.f, angMoveReverse);
				pCmd->forwardmove = -flLength;
				pCmd->sidemove = 0.f;
				pCmd->viewangles.y = fmodf(pCmd->viewangles.y - angMoveReverse.y, 360.f);
				pCmd->viewangles.z = 270.f;
				G::PSilentAngles = true;
			}
		}
		else
			pCmd->viewangles.x = 90.f;

		G::SilentAngles = true;
	}
}

void CMisc::FastMovement(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!pLocal->m_hGroundEntity() || pLocal->InCond(TF_COND_HALLOWEEN_KART))
		return;

	const float flSpeed = pLocal->m_vecVelocity().Length2D();
	const int flMaxSpeed = std::min(pLocal->m_flMaxspeed() * 0.9f, 520.f) - 10.f;
	const int iRun = !pCmd->forwardmove && !pCmd->sidemove ? 0 : flSpeed < flMaxSpeed ? 1 : 2;

	switch (iRun)
	{
	case 0:
	{
		if (!Vars::Misc::Movement::FastStop.Value || !flSpeed)
			return;

		Vec3 vDirection = pLocal->m_vecVelocity().ToAngle();
		vDirection.y = pCmd->viewangles.y - vDirection.y;
		Vec3 vNegatedDirection = vDirection.FromAngle() * -flSpeed;
		pCmd->forwardmove = vNegatedDirection.x;
		pCmd->sidemove = vNegatedDirection.y;

		break;
	}
	case 1:
	{
		if ((pLocal->IsDucking() ? !Vars::Misc::Movement::CrouchSpeed.Value : !Vars::Misc::Movement::FastAccel.Value)
			|| Vars::Misc::Game::AntiCheatCompatibility.Value
			|| G::Attacking == 1 || F::Ticks.m_bDoubletap || F::Ticks.m_bSpeedhack || F::Ticks.m_bRecharge || G::AntiAim || I::GlobalVars->tickcount % 2)
			return;

		if (!(pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)))
			return;

		Vec3 vMove = { pCmd->forwardmove, pCmd->sidemove, 0.f };
		float flLength = vMove.Length();
		Vec3 vAngMoveReverse; Math::VectorAngles(vMove * -1.f, vAngMoveReverse);
		pCmd->forwardmove = -flLength;
		pCmd->sidemove = 0.f;
		pCmd->viewangles.y = fmodf(pCmd->viewangles.y - vAngMoveReverse.y, 360.f);
		pCmd->viewangles.z = 270.f;
		G::PSilentAngles = true;

		break;
	}
	}
}

void CMisc::EdgeJump(CTFPlayer* pLocal, CUserCmd* pCmd, bool bPost)
{
	if (!Vars::Misc::Movement::EdgeJump.Value)
		return;

	static bool bStaticGround = false;
	if (!bPost)
		bStaticGround = pLocal->m_hGroundEntity();
	else if (bStaticGround && !pLocal->m_hGroundEntity())
		pCmd->buttons |= IN_JUMP;
}



void CMisc::Event(IGameEvent* pEvent, uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("client_disconnect"):
	case FNV1A::Hash32Const("client_beginconnect"):
	case FNV1A::Hash32Const("game_newmap"):
		m_iWishCmdrate = m_iWishUpdaterate = -1;
		F::Backtrack.m_flWishInterp = -1.f;
		[[fallthrough]];
	case FNV1A::Hash32Const("teamplay_round_start"):
		G::LineStorage.clear();
		G::BoxStorage.clear();
		G::PathStorage.clear();
		break;
	case FNV1A::Hash32Const("player_spawn"):
		m_bPeekPlaced = false;
		break;
	case FNV1A::Hash32Const("player_death"):
		AutoDisguiseOnBackstab(pEvent);
		break;
	}
}

int CMisc::AntiBackstab(CTFPlayer* pLocal, CUserCmd* pCmd, bool bSendPacket)
{
	if (!Vars::Misc::Automation::AntiBackstab.Value || !bSendPacket || G::Attacking == 1 || !pLocal || pLocal->m_MoveType() != MOVETYPE_WALK || pLocal->InCond(TF_COND_HALLOWEEN_KART))
		return 0;

	std::vector<std::pair<Vec3, CBaseEntity*>> vTargets = {};
	for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (pPlayer->IsDormant() || !pPlayer->IsAlive() || pPlayer->IsAGhost() || pPlayer->InCond(TF_COND_STEALTHED))
			continue;

		auto pWeapon = pPlayer->m_hActiveWeapon().Get()->As<CTFWeaponBase>();
		if (!pWeapon
			|| pWeapon->GetWeaponID() != TF_WEAPON_KNIFE
			&& !(G::PrimaryWeaponType == EWeaponType::MELEE && SDK::AttribHookValue(0, "crit_from_behind", pWeapon) > 0)
			&& !(pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER && SDK::AttribHookValue(0, "set_flamethrower_back_crit", pWeapon) == 1)
			|| F::PlayerUtils.IsIgnored(pPlayer->entindex()))
			continue;

		Vec3 vLocalPos = pLocal->GetCenter();
		Vec3 vTargetPos1 = pPlayer->GetCenter();
		Vec3 vTargetPos2 = vTargetPos1 + pPlayer->m_vecVelocity() * F::Backtrack.GetReal();
		float flDistance = std::max(std::max(SDK::MaxSpeed(pPlayer), SDK::MaxSpeed(pLocal)), pPlayer->m_vecVelocity().Length());
		if ((vLocalPos.DistTo(vTargetPos1) > flDistance || !SDK::VisPosWorld(pLocal, pPlayer, vLocalPos, vTargetPos1))
			&& (vLocalPos.DistTo(vTargetPos2) > flDistance || !SDK::VisPosWorld(pLocal, pPlayer, vLocalPos, vTargetPos2)))
			continue;

		vTargets.emplace_back(vTargetPos2, pEntity);
	}
	if (vTargets.empty())
		return 0;

	std::sort(vTargets.begin(), vTargets.end(), [&](const auto& a, const auto& b) -> bool
		{
			return pLocal->GetCenter().DistTo(a.first) < pLocal->GetCenter().DistTo(b.first);
		});

	auto& pTargetPos = vTargets.front();
	switch (Vars::Misc::Automation::AntiBackstab.Value)
	{
	case Vars::Misc::Automation::AntiBackstabEnum::Yaw:
	{
		Vec3 vAngleTo = Math::CalcAngle(pLocal->m_vecOrigin(), pTargetPos.first);
		vAngleTo.x = pCmd->viewangles.x;
		SDK::FixMovement(pCmd, vAngleTo);
		pCmd->viewangles = vAngleTo;
		
		return 1;
	}
	case Vars::Misc::Automation::AntiBackstabEnum::Pitch:
	case Vars::Misc::Automation::AntiBackstabEnum::Fake:
	{
		bool bCheater = F::PlayerUtils.HasTag(pTargetPos.second->entindex(), F::PlayerUtils.TagToIndex(CHEATER_TAG));
		// if the closest spy is a cheater, assume auto stab is being used, otherwise don't do anything if target is in front
		if (!bCheater)
		{
			auto TargetIsBehind = [&]()
				{
					Vec3 vToTarget = pLocal->m_vecOrigin() - pTargetPos.first;
					vToTarget.z = 0.f;
					const float flDist = vToTarget.Length();
					if (!flDist)
						return true;

					Vec3 vTargetAngles = pCmd->viewangles;

					vToTarget.Normalize();
					float flTolerance = 0.0625f;
					float flExtra = 2.f * flTolerance / flDist; // account for origin tolerance

					float flPosVsTargetViewMinDot = 0.f - 0.0031f - flExtra;

					Vec3 vTargetForward; Math::AngleVectors(vTargetAngles, &vTargetForward);
					vTargetForward.z = 0.f;
					vTargetForward.Normalize();

					return vToTarget.Dot(vTargetForward) > flPosVsTargetViewMinDot;
				};

			if (!TargetIsBehind())
				return 0;
		}

		if (!bCheater || Vars::Misc::Automation::AntiBackstab.Value == Vars::Misc::Automation::AntiBackstabEnum::Pitch)
		{
			pCmd->forwardmove *= -1;
			pCmd->viewangles.x = 269.f;
		}
		else
		{
			pCmd->viewangles.x = 271.f;
		}
		// may slip up some auto backstabs depending on mode, though we are still able to be stabbed

		return 2;
	}
	}

	return 0;
}

void CMisc::UnlockAchievements()
{
	const auto pAchievementMgr = reinterpret_cast<IAchievementMgr*(*)(void)>(U::Memory.GetVFunc(I::EngineClient, 114))();
	if (pAchievementMgr)
	{
		I::SteamUserStats->RequestCurrentStats();
		for (int i = 0; i < pAchievementMgr->GetAchievementCount(); i++)
			pAchievementMgr->AwardAchievement(pAchievementMgr->GetAchievementByIndex(i)->GetAchievementID());
		I::SteamUserStats->StoreStats();
		I::SteamUserStats->RequestCurrentStats();
	}
}

void CMisc::LockAchievements()
{
	const auto pAchievementMgr = reinterpret_cast<IAchievementMgr*(*)(void)>(U::Memory.GetVFunc(I::EngineClient, 114))();
	if (pAchievementMgr)
	{
		I::SteamUserStats->RequestCurrentStats();
		for (int i = 0; i < pAchievementMgr->GetAchievementCount(); i++)
			I::SteamUserStats->ClearAchievement(pAchievementMgr->GetAchievementByIndex(i)->GetName());
		I::SteamUserStats->StoreStats();
		I::SteamUserStats->RequestCurrentStats();
	}
}

bool CMisc::SteamRPC()
{
	/*
	if (!Vars::Misc::Steam::EnableRPC.Value)
	{
		if (!bSteamCleared) // stupid way to return back to normal rpc
		{
			I::SteamFriends->SetRichPresence("steam_display", ""); // this will only make it say "Team Fortress 2" until the player leaves/joins some server. its bad but its better than making 1000 checks to recreate the original
			bSteamCleared = true;
		}
		return false;
	}

	bSteamCleared = false;
	*/


	if (!Vars::Misc::Steam::EnableRPC.Value)
		return false;

	I::SteamFriends->SetRichPresence("steam_display", "#TF_RichPresence_Display");
	if (!I::EngineClient->IsInGame() && !Vars::Misc::Steam::OverrideMenu.Value)
		I::SteamFriends->SetRichPresence("state", "MainMenu");
	else
	{
		I::SteamFriends->SetRichPresence("state", "PlayingMatchGroup");

		switch (Vars::Misc::Steam::MatchGroup.Value)
		{
		case Vars::Misc::Steam::MatchGroupEnum::SpecialEvent: I::SteamFriends->SetRichPresence("matchgrouploc", "SpecialEvent"); break;
		case Vars::Misc::Steam::MatchGroupEnum::MvMMannUp: I::SteamFriends->SetRichPresence("matchgrouploc", "MannUp"); break;
		case Vars::Misc::Steam::MatchGroupEnum::Competitive: I::SteamFriends->SetRichPresence("matchgrouploc", "Competitive6v6"); break;
		case Vars::Misc::Steam::MatchGroupEnum::Casual: I::SteamFriends->SetRichPresence("matchgrouploc", "Casual"); break;
		case Vars::Misc::Steam::MatchGroupEnum::MvMBootCamp: I::SteamFriends->SetRichPresence("matchgrouploc", "BootCamp"); break;
		}
	}
	I::SteamFriends->SetRichPresence("currentmap", Vars::Misc::Steam::MapText.Value.c_str());
	I::SteamFriends->SetRichPresence("steam_player_group_size", std::to_string(Vars::Misc::Steam::GroupSize.Value).c_str());

	return true;
}

void CMisc::AutoDisguiseOnBackstab(IGameEvent* pEvent)
{
	// Skip if the feature is disabled
	if (!Vars::Misc::Automation::AutoDisguiseOnBackstab.Value)
		return;

	// Debug output - always print this message to check if function is being called
	I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Event received\"");
	
	// Validate event
	if (!pEvent)
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Event is null!\"");
		return;
	}

	char eventName[64];
	try {
		strcpy_s(eventName, pEvent->GetName());
		char debugMsg[128];
		sprintf_s(debugMsg, "echo \"Auto Disguise: Event name = %s\"", eventName);
		I::EngineClient->ClientCmd_Unrestricted(debugMsg);
	}
	catch (...) {
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Failed to get event name\"");
	}

	// Get the local player
	CTFPlayer* pLocal = H::Entities.GetLocal()->As<CTFPlayer>();
	if (!pLocal || !pLocal->IsAlive())
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Local player not available\"");
		return;
	}

	// Verify that the kill was done by the local player
	const int iAttacker = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
	const int localPlayerIdx = I::EngineClient->GetLocalPlayer();
	
	char debugMsg[128];
	sprintf_s(debugMsg, "echo \"Auto Disguise: Attacker=%d, Local=%d\"", iAttacker, localPlayerIdx);
	I::EngineClient->ClientCmd_Unrestricted(debugMsg);
	
	if (iAttacker != localPlayerIdx)
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Not our kill\"");
		return;
	}

	// Check if it was a backstab
	const int iCustomKill = pEvent->GetInt("customkill");
	sprintf_s(debugMsg, "echo \"Auto Disguise: CustomKill=%d, Need=%d\"", iCustomKill, TF_DMG_CUSTOM_BACKSTAB);
	I::EngineClient->ClientCmd_Unrestricted(debugMsg);
	
	if (iCustomKill != TF_DMG_CUSTOM_BACKSTAB)
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Not a backstab\"");
		return;
	}

	I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Backstab confirmed!\"");

	// Get the victim
	const int iVictim = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));
	IClientEntity* pVictimEntity = I::ClientEntityList->GetClientEntity(iVictim);
	if (!pVictimEntity)
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Victim entity not found\"");
		return;
	}

	CTFPlayer* pVictim = pVictimEntity->As<CTFPlayer>();
	if (!pVictim)
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Victim is not a player\"");
		return;
	}

	// Get the local player's active weapon
	CTFWeaponBase* pWeapon = pLocal->m_hActiveWeapon().Get()->As<CTFWeaponBase>();
	if (!pWeapon || pWeapon->GetWeaponID() != TF_WEAPON_KNIFE)
	{
		sprintf_s(debugMsg, "echo \"Auto Disguise: Not using knife, WeaponID=%d\"", 
			pWeapon ? pWeapon->GetWeaponID() : -1);
		I::EngineClient->ClientCmd_Unrestricted(debugMsg);
		return;
	}

	// Check if the knife is Your Eternal Reward - this knife auto-disguises, so we skip
	if (pWeapon->m_iItemDefinitionIndex() == Spy_t_YourEternalReward)
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Skipping - using YER\"");
		return;
	}

	I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Processing disguise...\"");

	// Get the victim's class
	const int iVictimClass = pVictim->m_iClass();
	sprintf_s(debugMsg, "echo \"Auto Disguise: Victim class=%d\"", iVictimClass);
	I::EngineClient->ClientCmd_Unrestricted(debugMsg);

	// Skip disguising as Heavy, Demoman, or Soldier if specified
	if (iVictimClass == 6 || iVictimClass == 4 || iVictimClass == 3) // 6=Heavy, 4=Demo, 3=Soldier
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Disguise: Victim is Heavy/Demo/Soldier, looking for alternatives...\"");
		
		// Try to find a nearby player to disguise as instead
		std::vector<std::pair<float, CBaseEntity*>> vNearbyPlayers;
		
		for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
		{
			auto pPlayer = pEntity->As<CTFPlayer>();
			if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsAGhost() || pPlayer == pVictim)
				continue;

			const int iClass = pPlayer->m_iClass();
			if (iClass == 6 || iClass == 4 || iClass == 3) // 6=Heavy, 4=Demo, 3=Soldier
				continue;
				
			const float flDistance = pLocal->m_vecOrigin().DistTo(pPlayer->m_vecOrigin());
			vNearbyPlayers.emplace_back(flDistance, pEntity);
		}

		// If we found any suitable players, disguise as the closest one
		if (!vNearbyPlayers.empty())
		{
			std::sort(vNearbyPlayers.begin(), vNearbyPlayers.end(),
				[](const auto& a, const auto& b) { return a.first < b.first; });

			auto pNearestPlayer = vNearbyPlayers.front().second->As<CTFPlayer>();
			if (pNearestPlayer)
			{
				// Disguise as the nearest player - use class number and opposite team (-1)
				char cmd[64];
				sprintf_s(cmd, "disguise %d -1", pNearestPlayer->m_iClass());
				I::EngineClient->ClientCmd_Unrestricted(cmd);
				
				char debugMsg[128];
				sprintf_s(debugMsg, "echo \"Auto Disguise: Disguising as nearby player class %d\"", pNearestPlayer->m_iClass());
				I::EngineClient->ClientCmd_Unrestricted(debugMsg);
				return;
			}
		}
		
		// If no suitable players were found, just disguise as a random non-excluded class
		int iClass = rand() % 9 + 1;  // Random class between 1-9
		while (iClass == 6 || iClass == 4 || iClass == 3) // 6=Heavy, 4=Demo, 3=Soldier
		{
			iClass = rand() % 9 + 1;
		}

		char cmd[64];
		sprintf_s(cmd, "disguise %d -1", iClass); // Use opposite team (-1)
		I::EngineClient->ClientCmd_Unrestricted(cmd);
		
		char debugMsg[128];
		sprintf_s(debugMsg, "echo \"Auto Disguise: Disguising as random class %d\"", iClass);
		I::EngineClient->ClientCmd_Unrestricted(debugMsg);
	}
	else
	{
		// Disguise as the victim's class using opposite team (-1)
		char cmd[64];
		sprintf_s(cmd, "disguise %d -1", iVictimClass);
		I::EngineClient->ClientCmd_Unrestricted(cmd);
		
		char debugMsg[128];
		sprintf_s(debugMsg, "echo \"Auto Disguise: Disguising as victim's class %d\"", iVictimClass);
		I::EngineClient->ClientCmd_Unrestricted(debugMsg);
	}
}

void CMisc::AutoVaccinator(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	// Skip if feature is disabled
	if (!Vars::Misc::Automation::AutoVaccinator.Value)
		return;

	// Static variables to maintain state across function calls
	static bool popInProgress = false;
	static bool switchInProgress = false;
	static int targetResistType = -1;
	static int switchAttempts = 0;
	static bool wasPressingReload = false;
	static bool wasPressingAttack2 = false;
	static Timer buttonCycleTimer = {};
	static Timer buttonPressTimer = {};

	// Debug output - much less frequent to avoid spam
	static Timer statusTimer = {};
	static Timer debugTimer = {};
	
	// Only log status every 20 seconds
	bool shouldLogStatus = statusTimer.Run(20.0f);
	if (shouldLogStatus)
	{
		I::EngineClient->ClientCmd_Unrestricted("echo \"Auto Vaccinator active and monitoring threats\"");
	}

	// Check if player is alive and is a Medic
	if (!pLocal || !pLocal->IsAlive() || pLocal->m_iClass() != 5) // 5 = Medic
	{
		return;
	}

	// Get the active weapon - add null check before dereferencing
	CTFWeaponBase* pWeapon = nullptr;
	CBaseEntity* activeWeaponEntity = pLocal->m_hActiveWeapon().Get();
	if (activeWeaponEntity)
	{
		pWeapon = activeWeaponEntity->As<CTFWeaponBase>();
	}
	
	if (!pWeapon)
	{
		return;
	}

	// Check if the weapon is a Vaccinator (item definition index: Medic_s_TheVaccinator = 998)
	// Make sure it's a medigun AND it's the vaccinator
	if (pWeapon->GetWeaponID() != TF_WEAPON_MEDIGUN || pWeapon->m_iItemDefinitionIndex() != 998)
	{
		return;
	}

	// Cast to CWeaponMedigun for medigun-specific properties
	auto pMedigun = pWeapon->As<CWeaponMedigun>();
	if (!pMedigun)
	{
		return;
	}

	// Check if medigun is healing someone - add null check before dereferencing
	CBaseEntity* targetEntity = pMedigun->m_hHealingTarget().Get();
	bool isHealing = false;
	if (targetEntity)
	{
		CTFPlayer* healTarget = targetEntity->As<CTFPlayer>();
		isHealing = healTarget && healTarget->IsAlive();
	}

	// Get current resistance type (0 = bullet, 1 = blast, 2 = fire)
	int iCurrentResistType = pMedigun->m_nChargeResistType();

    // Check if we have a charge ready - important for uber popping
	bool bChargeReady = pMedigun->m_bChargeRelease();

	// NEW: Check for scoped snipers aiming at us - highest priority defense
	bool bSniperThreat = false;
	bool bHeadshot = false;
	float flClosestSniperDistance = 9999.0f;
	Vec3 vSniperPosition;
	
	// Get local player head position for aim checks
	Vec3 vLocalHead = pLocal->GetEyePosition();
	
	// Get healing target head position if applicable
	Vec3 vTargetHead;
	CTFPlayer* pHealTarget = nullptr;
	
	if (isHealing && targetEntity)
	{
		pHealTarget = targetEntity->As<CTFPlayer>();
		if (pHealTarget && pHealTarget->IsAlive())
		{
			vTargetHead = pHealTarget->GetEyePosition();
		}
	}
	
	// Scan enemies specifically for scoped snipers
	for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsAGhost() || 
		   (pPlayer->IsDormant() && !H::Entities.GetDormancy(pPlayer->entindex())))
			continue;
		
		// Only interested in Snipers (TF_CLASS_SNIPER = 2)
		if (pPlayer->m_iClass() != 2)
			continue;
		
		// Check if sniper is zoomed in - TF_COND_ZOOMED = 13
		if (!pPlayer->InCond(TF_COND_ZOOMED))
			continue;
		
		// Calculate distance
		float flDistance = pLocal->m_vecOrigin().DistTo(pPlayer->m_vecOrigin());
		if (flDistance > 3000.0f) // Only care about snipers in reasonable range
			continue;
		
		// Get sniper's eye position and view angles
		Vec3 vSniperEye = pPlayer->GetEyePosition();
		Vec3 vSniperAngle = pPlayer->GetEyeAngles(); // Changed m_angEyeAngles to GetEyeAngles
		
		// Convert angle to forward vector
		Vec3 vSniperForward;
		Math::AngleVectors(vSniperAngle, &vSniperForward);
		
		// Check if aiming at local player
		Vec3 vDirToLocal = vLocalHead - vSniperEye;
		vDirToLocal.Normalize();
		float flDotLocal = vDirToLocal.Dot(vSniperForward);
		
		// High value means sniper is looking directly at us (0.98+ is very accurate aim)
		bool bAimingAtLocal = flDotLocal > 0.95f;
		
		// Check if they're aiming at head level (higher risk of headshot)
		float flLocalHeightDiff = fabs(vSniperAngle.x - Math::CalcAngle(vSniperEye, vLocalHead).x);
		bool bHeadshotLocal = flLocalHeightDiff < 2.0f;
		
		// Check if aiming at heal target
		bool bAimingAtTarget = false;
		bool bHeadshotTarget = false;
		
		if (pHealTarget)
		{
			Vec3 vDirToTarget = vTargetHead - vSniperEye;
			vDirToTarget.Normalize();
			float flDotTarget = vDirToTarget.Dot(vSniperForward);
			bAimingAtTarget = flDotTarget > 0.95f;
			
			// Check if they're aiming at heal target's head level
			float flTargetHeightDiff = fabs(vSniperAngle.x - Math::CalcAngle(vSniperEye, vTargetHead).x);
			bHeadshotTarget = flTargetHeightDiff < 2.0f;
		}
		
		// If sniper is aiming at either local player or heal target
		if (bAimingAtLocal || bAimingAtTarget)
		{
			bSniperThreat = true;
			
			// Prioritize headshot threats
			if ((bAimingAtLocal && bHeadshotLocal) || (bAimingAtTarget && bHeadshotTarget))
			{
				bHeadshot = true;
			}
			
			// Track closest threatening sniper
			if (flDistance < flClosestSniperDistance)
			{
				flClosestSniperDistance = flDistance;
				vSniperPosition = pPlayer->m_vecOrigin();
			}
		}
	}
	
	// Immediate action if sniper is aiming at us or our heal target
	if (bSniperThreat && !popInProgress && !switchInProgress)
	{
		char sniperMsg[128];
		sprintf_s(sniperMsg, "echo \"Vacc: SNIPER THREAT DETECTED! Headshot=%s Distance=%.0f\"", 
			bHeadshot ? "YES" : "no", flClosestSniperDistance);
		I::EngineClient->ClientCmd_Unrestricted(sniperMsg);
		
		// If we're not already on bullet resistance, switch immediately
		if (iCurrentResistType != 0) // 0 = bullet
		{
			targetResistType = 0; // Bullet resistance
			switchInProgress = true;
			buttonCycleTimer.Update();
			buttonPressTimer.Update();
			switchAttempts = 0;
			wasPressingReload = (pCmd->buttons & IN_RELOAD);
			
			I::EngineClient->ClientCmd_Unrestricted("echo \"Vacc: EMERGENCY SWITCH to bullet resistance!\"");
		}
		// If we're already on bullet resistance and charge is ready, pop uber immediately
		else if (bChargeReady)
		{
			popInProgress = true;
			buttonCycleTimer.Update();
			wasPressingAttack2 = (pCmd->buttons & IN_ATTACK2);
			
			// Use more urgent message for headshot threats
			if (bHeadshot)
				I::EngineClient->ClientCmd_Unrestricted("echo \"Vacc: EMERGENCY POP for HEADSHOT threat!\"");
			else
				I::EngineClient->ClientCmd_Unrestricted("echo \"Vacc: EMERGENCY POP for sniper threat!\"");
		}
	}
	
	// NEW: Check for burning condition on self or healing target - emergency fire resistance
	bool isBurning = pLocal->InCond(TF_COND_BURNING) || pLocal->InCond(TF_COND_BURNING_PYRO);
	bool isTargetBurning = false;
	
	if (isHealing && targetEntity)
	{
		CTFPlayer* healTarget = targetEntity->As<CTFPlayer>();
		if (healTarget)
		{
			isTargetBurning = healTarget->InCond(TF_COND_BURNING) || healTarget->InCond(TF_COND_BURNING_PYRO);
		}
	}
	
	// Immediate action for burning - this takes precedence over other threats
	if ((isBurning || isTargetBurning) && !popInProgress && !switchInProgress)
	{
		// Log the burning condition
		if (debugTimer.Run(0.5f))
		{
			char burnMsg[128];
			sprintf_s(burnMsg, "echo \"Vacc EMERGENCY: %s burning - switching to fire resistance!\"", 
				isBurning ? (isTargetBurning ? "Both players are" : "Medic is") : "Target is");
			I::EngineClient->ClientCmd_Unrestricted(burnMsg);
		}
		
		// If we're not on fire resistance already, switch to it immediately
		if (iCurrentResistType != 2)
		{
			targetResistType = 2; // Fire resistance
			switchInProgress = true;
			buttonCycleTimer.Update();
			buttonPressTimer.Update();
			switchAttempts = 0;
			wasPressingReload = (pCmd->buttons & IN_RELOAD);
			
			I::EngineClient->ClientCmd_Unrestricted("echo \"Vacc: EMERGENCY SWITCH to fire resistance!\"");
		}
		// If we have charge ready and are on fire resistance, pop uber immediately
		else if (bChargeReady)
		{
			popInProgress = true;
			buttonCycleTimer.Update();
			wasPressingAttack2 = (pCmd->buttons & IN_ATTACK2);
			
			I::EngineClient->ClientCmd_Unrestricted("echo \"Vacc: EMERGENCY POP for burning players!\"");
		}
	}

	// Only log significant state changes (not every tick)
	static int lastResistType = -1;
	static bool wasHealing = false;
	static Timer resistanceChangeTimer = {};
	
	if ((lastResistType != iCurrentResistType || wasHealing != isHealing) && debugTimer.Run(1.0f))
	{
		if (lastResistType != iCurrentResistType) 
		{
			// Track when resistance actually changes
			resistanceChangeTimer.Update();
		}
		
		char debugMsg[128];
		sprintf_s(debugMsg, "echo \"Vaccinator: Resistance=%s, Healing=%s\"", 
			iCurrentResistType == 0 ? "Bullet" : (iCurrentResistType == 1 ? "Blast" : "Fire"),
			isHealing ? "Yes" : "No");
		I::EngineClient->ClientCmd_Unrestricted(debugMsg);
		
		lastResistType = iCurrentResistType;
		wasHealing = isHealing;
	}

	// Find nearby threats
	enum ThreatLevel {
		THREAT_NONE = 0,
		THREAT_NORMAL = 1,
		THREAT_MINICRITS = 2,
		THREAT_CRITS = 3
	};

	struct ThreatInfo {
		ThreatLevel level;
		int damageType; // 0 = bullet, 1 = blast, 2 = fire
		float distance;
		Vec3 position;
		int playerClass; // Store player class for better debugging
	};

	std::vector<ThreatInfo> threats;
	
	// Scan for enemies - more condensed code to reduce excessive debug info
	for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
	{
		if (!pEntity)
			continue;
		
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsAGhost() || 
		   (pPlayer->IsDormant() && !H::Entities.GetDormancy(pPlayer->entindex())))
			continue;

		// Calculate distance
		float flDistance = pLocal->m_vecOrigin().DistTo(pPlayer->m_vecOrigin());
		
		// Skip players too far away (beyond reasonable threat range)
		if (flDistance > 1000.0f)
			continue;

		// Get weapon - handle dormant players
		CTFWeaponBase* pEnemyTFWeapon = nullptr;
		int weaponID = 0;
		
		if (!pPlayer->IsDormant())
		{
			CBaseEntity* pEnemyWeaponEntity = pPlayer->m_hActiveWeapon().Get();
			if (pEnemyWeaponEntity)
			{
				pEnemyTFWeapon = pEnemyWeaponEntity->As<CTFWeaponBase>();
				if (pEnemyTFWeapon)
				{
					weaponID = pEnemyTFWeapon->GetWeaponID();
				}
			}
		}

		// Determine threat level and damage type
		ThreatLevel level = THREAT_NORMAL;
		int damageType = 0; // Default to bullets

		// Check for crits
		if (pPlayer->InCond(TF_COND_CRITBOOSTED) || 
			pPlayer->InCond(TF_COND_CRITBOOSTED_DEMO_CHARGE) ||
			pPlayer->InCond(TF_COND_CRITBOOSTED_PUMPKIN) ||
			pPlayer->InCond(TF_COND_CRITBOOSTED_FIRST_BLOOD) ||
			pPlayer->InCond(TF_COND_CRITBOOSTED_BONUS_TIME) ||
			pPlayer->InCond(TF_COND_CRITBOOSTED_CTF_CAPTURE) ||
			pPlayer->InCond(TF_COND_CRITBOOSTED_ON_KILL) ||
			pPlayer->InCond(TF_COND_CRITBOOSTED_CARD_EFFECT) ||
			pPlayer->InCond(TF_COND_CRITBOOSTED_RUNE_TEMP))
		{
			level = THREAT_CRITS;
		}
		// Check for mini-crits
		else if (pPlayer->InCond(TF_COND_OFFENSEBUFF) ||
				pPlayer->InCond(TF_COND_MINICRITBOOSTED_ON_KILL))
		{
			level = THREAT_MINICRITS;
		}

		// If we know the class, use that for initial damage type guess
		switch (pPlayer->m_iClass())
		{
			case TF_CLASS_SOLDIER:
			case TF_CLASS_DEMOMAN:
				damageType = 1; // Explosive by default
				break;
			case TF_CLASS_PYRO:
				damageType = 2; // Fire by default
				break;
			default:
				damageType = 0; // Bullet by default
		}
		
		// If we have the weapon, get more specific damage type
		if (pEnemyTFWeapon)
		{
		// Explosive weapons
		if (weaponID == TF_WEAPON_ROCKETLAUNCHER || 
			weaponID == TF_WEAPON_GRENADELAUNCHER ||
			weaponID == TF_WEAPON_PIPEBOMBLAUNCHER ||
			weaponID == TF_WEAPON_STICKBOMB ||
				weaponID == TF_WEAPON_PARTICLE_CANNON ||
				weaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT)
		{
			damageType = 1; // Explosive
		}
		// Fire weapons
			else if (weaponID == TF_WEAPON_FLAMETHROWER)
		{
			damageType = 2; // Fire
		}
		}
		
		// Always consider burning players as fire threats
		if (pPlayer->InCond(TF_COND_BURNING) || pPlayer->InCond(TF_COND_BURNING_PYRO))
		{
			damageType = 2; // Fire
		}
		
		// Add to threats list
		threats.push_back({level, damageType, flDistance, pPlayer->m_vecOrigin(), pPlayer->m_iClass()});
	}

	// Count enemies by type for debug and analysis
	int bulletEnemies = 0, blastEnemies = 0, fireEnemies = 0;
	for (const auto& threat : threats)
	{
		if (threat.damageType == 0) bulletEnemies++;
		else if (threat.damageType == 1) blastEnemies++;
		else if (threat.damageType == 2) fireEnemies++;
	}
	
	// Log enemy count info without spamming
	static Timer enemyCountTimer = {};
	if (enemyCountTimer.Run(5.0f))
	{
		char debugMsg[128];
		sprintf_s(debugMsg, "echo \"Vacc Status: Detected %d enemies - Bullet:%d, Blast:%d, Fire:%d\"", 
			bulletEnemies + blastEnemies + fireEnemies, bulletEnemies, blastEnemies, fireEnemies);
		I::EngineClient->ClientCmd_Unrestricted(debugMsg);
	}

	// Process threats
	ThreatInfo highestThreat = {THREAT_NONE, 0, 0.0f, Vec3(), 0};
	
	// Find the highest threat
	for (const auto& threat : threats)
	{
		if (threat.level > highestThreat.level || 
			(threat.level == highestThreat.level && threat.distance < highestThreat.distance))
		{
			highestThreat = threat;
		}
	}
	
	// Static variables for human-like reaction
	static Timer reactionTimer = {};
	static bool reacting = false;
	static ThreatInfo currentReactionThreat = {THREAT_NONE, 0, 0.0f, Vec3(), 0};
	
	// If we have a significant threat (mini-crits or higher)
	if (highestThreat.level >= THREAT_MINICRITS)
	{
		// Debug output for threat detection only when significant changes happen
		if (currentReactionThreat.level != highestThreat.level || 
		    currentReactionThreat.damageType != highestThreat.damageType)
		{
			char debugMsg[128];
			const char* threatType = highestThreat.damageType == 0 ? "Bullet" : 
									(highestThreat.damageType == 1 ? "Blast" : "Fire");
			sprintf_s(debugMsg, "echo \"Threat detected: %s (%s), Level=%d, Dist=%.0f\"", 
				SDK::GetClassByIndex(highestThreat.playerClass),
				threatType,
				highestThreat.level, 
				highestThreat.distance);
			I::EngineClient->ClientCmd_Unrestricted(debugMsg);
		}
	
		// If we're not already reacting to a threat, start reaction timer
		if (!reacting)
		{
			currentReactionThreat = highestThreat;
			reacting = true;
			reactionTimer.Update();
		}
		
		// Check if the current threat is different and more severe
		if (highestThreat.level > currentReactionThreat.level)
		{
			currentReactionThreat = highestThreat;
			reactionTimer.Update();
		}
		
		// Human reaction time (between 150-350ms for most people)
		float reactionTime = 0.05f; // Reduced from 0.2f for faster reactions
		
		// Increase reaction time slightly for minicrits (less urgent)
		if (highestThreat.level == THREAT_MINICRITS)
			reactionTime = 0.1f; // Reduced from 0.3f
			
		// If reaction time has passed
		if (reactionTimer.Check(reactionTime))
		{
			// Pop uber if the resistance type matches the threat
			// Or if it's critical threat, pop regardless
			if ((iCurrentResistType == currentReactionThreat.damageType || currentReactionThreat.level == THREAT_CRITS) 
			    && bChargeReady && !popInProgress && !switchInProgress)
			{
				// Start popping process
				popInProgress = true;
				buttonCycleTimer.Update();
				wasPressingAttack2 = (pCmd->buttons & IN_ATTACK2); // Store current button state
				
				char popMsg[128];
				sprintf_s(popMsg, "echo \"POPPING UBER for %s threat! (Charge: %s, Type: %d)\"", 
					currentReactionThreat.level == THREAT_CRITS ? "CRITICAL" : "high",
					bChargeReady ? "READY" : "not ready", 
					iCurrentResistType);
				I::EngineClient->ClientCmd_Unrestricted(popMsg);
			}
			// Otherwise, if threat is high enough, switch resistance type
			else if (currentReactionThreat.level >= THREAT_MINICRITS && iCurrentResistType != currentReactionThreat.damageType 
			         && !popInProgress && !switchInProgress)
			{
				// Start resistance switching process
				targetResistType = currentReactionThreat.damageType;
				switchInProgress = true;
				buttonCycleTimer.Update();
				buttonPressTimer.Update();
				switchAttempts = 0;
				wasPressingReload = (pCmd->buttons & IN_RELOAD); // Store current button state
				
				const char* currentType = iCurrentResistType == 0 ? "Bullet" : (iCurrentResistType == 1 ? "Blast" : "Fire");
				const char* targetType = targetResistType == 0 ? "Bullet" : (targetResistType == 1 ? "Blast" : "Fire");
				
					char debugMsg[128];
				sprintf_s(debugMsg, "echo \"Cycling resistance: %s → %s for threat level %d\"", 
					currentType, targetType, currentReactionThreat.level);
					I::EngineClient->ClientCmd_Unrestricted(debugMsg);
			}
			
			reacting = false;
		}
	}
	else
	{
		// No current threat, reset reaction state
		if (reacting)
		{
			reacting = false;
		}
		
		// When no immediate threats, still analyze nearby enemies and switch to most appropriate resistance
		// This fixes the issue where it stays on the first resistance type
		static Timer cycleTimer = {};

		// More frequent cycling when not healing (but don't spam)
		if (!isHealing && !switchInProgress && !popInProgress && cycleTimer.Run(1.5f)) // Reduced from 3.0f
		{
			// --- RESTORED OLD THREAT SYSTEM ---
			// Calculate threat scores for each resistance type
			float threatScores[3] = {0.0f, 0.0f, 0.0f}; // bullet, blast, fire
			float threatDistances[3] = {9999.0f, 9999.0f, 9999.0f};
			
			// Scan for nearby enemies to determine best resistance
			for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
			{
				auto pPlayer = pEntity->As<CTFPlayer>();
				if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsAGhost())
					continue;
					
				// Skip dormant players unless we have dormant ESP enabled
				if (pPlayer->IsDormant() && !H::Entities.GetDormancy(pPlayer->entindex()))
					continue;

				// Calculate distance
				float flDistance = pLocal->m_vecOrigin().DistTo(pPlayer->m_vecOrigin());
				
				// Skip players too far away
				const float flMaxRange = 1200.0f; // Slightly higher than typical combat range
				if (flDistance > flMaxRange)
					continue;
					
				// Get player's active weapon
				CTFWeaponBase* pEnemyWeapon = nullptr;
				if (!pPlayer->IsDormant())
				{
					auto pWeaponEnt = pPlayer->m_hActiveWeapon().Get();
					if (pWeaponEnt)
						pEnemyWeapon = pWeaponEnt->As<CTFWeaponBase>();
				}

				// Determine threat type based on player class and weapon
				int primaryThreatType = 0; // Default to bullets
				float threatWeight = 1.0f; // Base threat weight
				
				// Class-based threat detection if weapon isn't available
				switch (pPlayer->m_iClass())
				{
					case TF_CLASS_DEMOMAN:
						primaryThreatType = 1; // Explosives
						break;
					case TF_CLASS_SOLDIER:
						primaryThreatType = 1; // Explosives
						break;
					case TF_CLASS_PYRO:
						primaryThreatType = 2; // Fire
						break;
					// All other classes default to bullet damage
				}
				
				// If we have weapon info, use that for more accurate detection
				if (pEnemyWeapon)
				{
					int weaponID = pEnemyWeapon->GetWeaponID();
					
					// Explosive weapons
					if (weaponID == TF_WEAPON_ROCKETLAUNCHER || 
						weaponID == TF_WEAPON_GRENADELAUNCHER ||
						weaponID == TF_WEAPON_PIPEBOMBLAUNCHER ||
						weaponID == TF_WEAPON_STICKBOMB ||
						weaponID == TF_WEAPON_PARTICLE_CANNON ||
						weaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT)
					{
						primaryThreatType = 1; // Explosive
						
						// Special case for sticky bombs - higher priority if stickies are out
						if (weaponID == TF_WEAPON_PIPEBOMBLAUNCHER || weaponID == TF_WEAPON_STICKBOMB)
						{
							// Check if player has stickies deployed
							for (auto proj : H::Entities.GetGroup(EGroupType::WORLD_PROJECTILES))
							{
								if (proj->GetClassID() == ETFClassID::CTFGrenadePipebombProjectile)
								{
									auto pipebomb = proj->As<CTFGrenadePipebombProjectile>();
									if (pipebomb && pipebomb->m_hThrower().Get() == pPlayer && 
										pipebomb->m_iType() == TF_GL_MODE_REMOTE_DETONATE)
									{
										threatWeight = 1.5f; // Higher weight for active stickies
										break;
									}
								}
							}
						}
					}
					// Fire weapons
					else if (weaponID == TF_WEAPON_FLAMETHROWER)
					{
						primaryThreatType = 2; // Fire
						
						// If pyro is very close, higher fire threat
						if (flDistance < 400.0f)
							threatWeight = 2.0f;
					}
				}
				
				// Always consider fire condition as fire threat
				if (pPlayer->InCond(TF_COND_BURNING) || pPlayer->InCond(TF_COND_BURNING_PYRO))
				{
					primaryThreatType = 2; // Fire
				}
				
				// Check if player has crits/minicrits - increase weight accordingly
				if (pPlayer->InCond(TF_COND_CRITBOOSTED) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_DEMO_CHARGE) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_PUMPKIN) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_FIRST_BLOOD) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_BONUS_TIME) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_CTF_CAPTURE) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_ON_KILL) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_CARD_EFFECT) ||
					pPlayer->InCond(TF_COND_CRITBOOSTED_RUNE_TEMP))
				{
					threatWeight *= 2.0f; // Double threat if crit boosted
				}
				else if (pPlayer->InCond(TF_COND_OFFENSEBUFF) || 
						pPlayer->InCond(TF_COND_MINICRITBOOSTED_ON_KILL))
				{
					threatWeight *= 1.5f; // 1.5x threat if mini-crit boosted
				}
				
				// Distance-based weighting (closer enemies are more threatening)
				float distanceFactor = 1.0f - (flDistance / flMaxRange);
				distanceFactor = std::max(0.1f, distanceFactor); // At least 0.1 weight
				
				// Final threat calculation
				float threatScore = threatWeight * distanceFactor;
				
				// Update threat type scores and distances
				threatScores[primaryThreatType] += threatScore;
				threatDistances[primaryThreatType] = std::min(threatDistances[primaryThreatType], flDistance);
			}
			
			// Additional check for projectiles in the world that might pose a threat
			for (auto pProjectile : H::Entities.GetGroup(EGroupType::WORLD_PROJECTILES))
			{
				if (!pProjectile)
					continue;
					
				float flDistance = pLocal->m_vecOrigin().DistTo(pProjectile->m_vecOrigin());
				if (flDistance > 600.0f) // Only care about closer projectiles
					continue;
					
				int projectileType = 0; // Default to bullet
				
				// Check projectile types
				switch (pProjectile->GetClassID())
				{
					case ETFClassID::CTFGrenadePipebombProjectile:
					case ETFClassID::CTFProjectile_Rocket:
					case ETFClassID::CTFProjectile_SentryRocket:
						projectileType = 1; // Explosive
						// Add to threat score for explosives (nearby projectiles are dangerous)
						threatScores[projectileType] += (1.0f - (flDistance / 600.0f)) * 2.0f;
						threatDistances[projectileType] = std::min(threatDistances[projectileType], flDistance);
						break;
						
					case ETFClassID::CTFProjectile_Flare:
						projectileType = 2; // Fire
						// Add to threat score for fire
						threatScores[projectileType] += (1.0f - (flDistance / 600.0f)) * 1.5f;
						threatDistances[projectileType] = std::min(threatDistances[projectileType], flDistance);
						break;
				}
			}
			
			// Determine the best resistance type based on threat scores
			float highestScore = 0.0f;
			targetResistType = -1; // -1 means no change needed
			
			// First check if we have threats at all
			bool hasThreats = false;
			for (int i = 0; i < 3; i++)
			{
				if (threatScores[i] > 0.0f)
				{
					hasThreats = true;
					if (threatScores[i] > highestScore)
					{
						highestScore = threatScores[i];
						targetResistType = i;
					}
				}
			}
			
			// If no threats detected, cycle to next resistance (but only when not healing)
			if (!hasThreats && !isHealing)
			{
				targetResistType = (iCurrentResistType + 1) % 3;
			}
			
			// Only switch if we need to and we're not already at this resistance type
			if (targetResistType >= 0 && targetResistType != iCurrentResistType)
			{
				// Start a resistance switch
				switchInProgress = true;
			buttonCycleTimer.Update();
				buttonPressTimer.Update();
				switchAttempts = 0;
				wasPressingReload = (pCmd->buttons & IN_RELOAD); // Store current button state
				
				// Only log changes to avoid spam
				const char* currentType = iCurrentResistType == 0 ? "Bullet" : (iCurrentResistType == 1 ? "Blast" : "Fire");
				const char* targetType = targetResistType == 0 ? "Bullet" : (targetResistType == 1 ? "Blast" : "Fire");
				
				char debugMsg[128];
				if (hasThreats)
				{
					sprintf_s(debugMsg, "echo \"Auto-cycling to %s (Scores: B:%.1f E:%.1f F:%.1f)\"", 
						targetType, threatScores[0], threatScores[1], threatScores[2]);
				}
				else
				{
					sprintf_s(debugMsg, "echo \"Cycling resistance: %s → %s (no threats)\"", 
						currentType, targetType);
				}
				I::EngineClient->ClientCmd_Unrestricted(debugMsg);
			}
		}
	}

	// --------- IMPROVED BUTTON PRESSING LOGIC ---------
	// Handle button press/release cycles with better verification
	if (switchInProgress)
	{
		// Check if resistance has changed since we started switching
		// If the resistance has changed and it matches our target, we're done
		if (targetResistType == iCurrentResistType)
		{
			switchInProgress = false;
			I::EngineClient->ClientCmd_Unrestricted("echo \"Vacc: Resistance switch successful!\"");
		}
		// If we've been trying to switch for too long, press the button multiple times
		else if (buttonPressTimer.Check(0.2f)) // Reduced from 0.5f
		{
			// We need to cycle 1 or 2 times to reach the target resistance from current
			int numPresses = (targetResistType - iCurrentResistType + 3) % 3;
			
			// Actually press the button the calculated number of times
			const int maxAttempts = 4; // Prevent infinite loops
			
			if (switchAttempts < maxAttempts)
			{
				// Press the reload button
				pCmd->buttons |= IN_RELOAD;
				
				// This is a distinct cycle
				switchAttempts++;
				
				// Only log this to reduce spam
				if (switchAttempts == 1 || switchAttempts == maxAttempts)
				{
					char debugMsg[128];
					sprintf_s(debugMsg, "echo \"Vacc: Pressing reload (attempt %d/%d) - need %d presses to go from %d to %d\"",
						switchAttempts, maxAttempts, numPresses, iCurrentResistType, targetResistType);
					I::EngineClient->ClientCmd_Unrestricted(debugMsg);
				}
				
				// Update the timer for next check
				buttonPressTimer.Update();
			}
			else
			{
				// We've tried enough times, give up
				switchInProgress = false;
				I::EngineClient->ClientCmd_Unrestricted("echo \"Vacc: Giving up on resistance switch after multiple attempts\"");
			}
		}
		else
		{
			// Release the button between presses (important for the game to register multiple presses)
			pCmd->buttons &= ~IN_RELOAD;
		}
	}
	
	if (popInProgress)
	{
		// First phase - press button for uber pop
		if (buttonCycleTimer.Check(0.0f))
		{
			// Force press M2 button - very important to set it directly
			pCmd->buttons |= IN_ATTACK2;
			
			// Log that we're pressing the button
			I::EngineClient->ClientCmd_Unrestricted("echo \"UBER POP: Pressing M2 button!\"");
			
			// Update timer for next phase
			buttonCycleTimer.Update();
		}
		// Second phase - release button
		else if (buttonCycleTimer.Check(0.05f)) // Faster release for more responsive popping
		{
			// Release the button
			pCmd->buttons &= ~IN_ATTACK2;
			I::EngineClient->ClientCmd_Unrestricted("echo \"UBER POP: Releasing M2 button\"");
			
			// End the pop sequence
			popInProgress = false;
		}
	}
	
	// Update tracking vars with current state if not in a cycle
	if (!popInProgress)
		wasPressingAttack2 = (pCmd->buttons & IN_ATTACK2);
}