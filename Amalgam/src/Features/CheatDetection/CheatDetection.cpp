#include "CheatDetection.h"

#include "../Players/PlayerUtils.h"
#include "../Output/Output.h"

bool CCheatDetection::ShouldScan()
{
	if (!Vars::CheatDetection::Methods.Value /*|| I::EngineClient->IsPlayingDemo()*/)
		return false;

	static int iStaticTickcount = I::GlobalVars->tickcount;
	const int iLastTickcount = iStaticTickcount;
	const int iCurrTickcount = iStaticTickcount = I::GlobalVars->tickcount;
	if (iCurrTickcount != iLastTickcount + 1)
		return false;

	auto pNetChan = I::EngineClient->GetNetChannelInfo();
	if (pNetChan && (pNetChan->GetTimeSinceLastReceived() > TICK_INTERVAL * 2 || pNetChan->IsTimingOut()))
		return false;

	return true;
}

bool CCheatDetection::InvalidPitch(CTFPlayer* pEntity)
{
	return Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::InvalidPitch && fabsf(pEntity->m_angEyeAnglesX()) == 90.f;
}

bool CCheatDetection::IsChoking(CTFPlayer* pEntity)
{
	auto& pc = mData[pEntity].m_PacketChoking;
	bool bReturn = pc.m_bInfract;
	pc.m_bInfract = false;

	return Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::PacketChoking && bReturn;
}

bool CCheatDetection::IsFlicking(CTFPlayer* pEntity) // awful
{
	auto& af = mData[pEntity].m_AimFlicking;
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::AimFlicking))
	{
		af.m_iAngleCount = 0;
		return false;
	}

	af.m_aAngles[2] = af.m_aAngles[1];
	af.m_aAngles[1] = af.m_aAngles[0];
	af.m_aAngles[0] = { pEntity->GetEyeAngles(), false };
	if (af.m_iAngleCount < 3)
		af.m_iAngleCount++;

	if (af.m_iAngleCount != 3 || !af.m_aAngles[0].m_bAttacking && !af.m_aAngles[1].m_bAttacking && !af.m_aAngles[2].m_bAttacking
		|| Math::CalcFov(af.m_aAngles[0].m_vAngle, af.m_aAngles[1].m_vAngle) < Vars::CheatDetection::MinFlick.Value
		|| Math::CalcFov(af.m_aAngles[0].m_vAngle, af.m_aAngles[2].m_vAngle) > Vars::CheatDetection::MaxNoise.Value * (TICK_INTERVAL / 0.015f))
		return false;

	af.m_iAngleCount = 0;
	return true;
}

bool CCheatDetection::IsDuckSpeed(CTFPlayer* pEntity)
{
	auto& ds = mData[pEntity].m_DuckSpeed;
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::DuckSpeed)
		|| !pEntity->IsDucking() || !pEntity->IsOnGround()
		|| pEntity->m_vecVelocity().Length2D() < pEntity->m_flMaxspeed() * 0.5f)
	{
		ds.m_iStartTick = 0;
		return false;
	}

	if (!ds.m_iStartTick)
		ds.m_iStartTick = I::GlobalVars->tickcount;

	if (I::GlobalVars->tickcount - ds.m_iStartTick > TIME_TO_TICKS(1))
	{
		ds.m_iStartTick = 0;
		return true;
	}

	return false;
}

void CCheatDetection::Infract(CTFPlayer* pEntity, const char* sReason)
{
	auto& info = mData[pEntity];
	bool bMark = false;
	if (Vars::CheatDetection::DetectionsRequired.Value)
	{
		info.m_iDetections++;
		bMark = info.m_iDetections >= Vars::CheatDetection::DetectionsRequired.Value;
	}

	F::Output.CheatDetection(info.m_sName, bMark ? "marked" : "infracted", sReason);
	if (bMark)
	{
		info.m_iDetections = 0;
		F::PlayerUtils.AddTag(info.m_uAccountID, F::PlayerUtils.TagToIndex(CHEATER_TAG), true, info.m_sName);
	}
}

void CCheatDetection::Run()
{
	if (!ShouldScan() || !I::EngineClient->IsConnected() || I::EngineClient->IsPlayingDemo())
		return;

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return;

	for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		int iIndex = pPlayer->entindex();
		if (!H::Entities.GetDeltaTime(iIndex))
			continue;

		if (iIndex == H::Entities.GetLocalPlayerIndex() || !pPlayer->IsAlive() || pPlayer->IsAGhost()
			|| pResource->IsFakePlayer(iIndex) || F::PlayerUtils.HasTag(iIndex, F::PlayerUtils.TagToIndex(CHEATER_TAG)))
		{
			auto& info = mData[pPlayer];
			if (info.m_PacketChoking.m_iChokeCount | info.m_AimFlicking.m_iAngleCount | info.m_DuckSpeed.m_iStartTick)
			{
				info.m_PacketChoking = {};
				info.m_AimFlicking = {};
				info.m_DuckSpeed = {};
			}
			continue;
		}

		auto& info = mData[pPlayer];
		info.m_uAccountID = pResource->m_iAccountID(iIndex);
		info.m_sName = F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex));

		if (InvalidPitch(pPlayer))
			Infract(pPlayer, "invalid pitch");
		if (IsChoking(pPlayer))
			Infract(pPlayer, "choking packets");
		if (IsFlicking(pPlayer))
			Infract(pPlayer, "flicking");
		if (IsDuckSpeed(pPlayer))
			Infract(pPlayer, "duck speed");
	}
}

void CCheatDetection::Reset()
{
	mData.clear();
}

void CCheatDetection::ReportChoke(CTFPlayer* pEntity, int iChoke)
{
	auto& pc = mData[pEntity].m_PacketChoking;
	if (Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::PacketChoking)
	{
		pc.m_iChokes[2] = pc.m_iChokes[1];
		pc.m_iChokes[1] = pc.m_iChokes[0];
		pc.m_iChokes[0] = iChoke;
		if (pc.m_iChokeCount < 3)
			pc.m_iChokeCount++;
		if (pc.m_iChokeCount == 3)
		{
			pc.m_bInfract = true; // check for last 3 choke amounts
			for (auto iVal : pc.m_iChokes)
			{
				if (iVal < Vars::CheatDetection::MinChoking.Value)
					pc.m_bInfract = false;
			}
			pc.m_iChokeCount = 0;
		}
	}
	else
		pc.m_iChokeCount = 0;
}

void CCheatDetection::ReportDamage(IGameEvent* pEvent)
{
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::AimFlicking))
		return;

	int iIndex = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
	if (iIndex == H::Entities.GetLocalPlayerIndex())
		return;

	auto pEntity = I::ClientEntityList->GetClientEntity(iIndex)->As<CTFPlayer>();
	if (!pEntity || !pEntity->IsPlayer() || pEntity->IsDormant())
		return;

	auto pWeapon = pEntity->m_hActiveWeapon()->As<CTFWeaponBase>();
	switch (SDK::GetWeaponType(pWeapon))
	{
	case EWeaponType::UNKNOWN:
	case EWeaponType::PROJECTILE:
		return;
	}

	auto& af = mData[pEntity].m_AimFlicking;
	if (af.m_iAngleCount > 0)
		af.m_aAngles[af.m_iAngleCount - 1].m_bAttacking = true;
}