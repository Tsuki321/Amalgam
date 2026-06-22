#include "Chams.h"

#include "../Groups/Groups.h"
#include "../Materials/Materials.h"
#include "../FakeAngle/FakeAngle.h"
#include "../../Backtrack/Backtrack.h"

void CChams::Begin()
{
	m_tOriginalColor = I::RenderView->GetColorModulation();
	m_flOriginalBlend = I::RenderView->GetBlend();
	I::ModelRender->GetMaterialOverride(&m_pOriginalMaterial, &m_iOriginalOverride);
}
void CChams::End()
{
	I::RenderView->SetColorModulation(m_tOriginalColor);
	I::RenderView->SetBlend(m_flOriginalBlend);
	I::ModelRender->ForcedMaterialOverride(m_pOriginalMaterial, m_iOriginalOverride);
}

void CChams::DrawModel(CBaseEntity* pEntity, const Chams_t& tChams, IMatRenderContext* pRenderContext, int iModel, bool bTwoModel, const std::vector<std::pair<uint32_t, Color_t>>* pVisibleHashes, const std::vector<std::pair<uint32_t, Color_t>>* pOccludedHashes)
{
	if (!m_iFlags && iModel == ModelEnum::Visible)
		m_mEntities[pEntity->entindex()];

	bool bOccluded = !tChams.Occluded.empty();
	bool bSame = tChams.Visible == tChams.Occluded;
	bTwoModel &= bOccluded && !bSame;

	Begin();
	switch (iModel)
	{
	case ModelEnum::Visible:
	{
		if (!bTwoModel)
		{
			if (bSame)
				return;
		}
		else
		{
			pRenderContext->SetStencilEnable(true);
			pRenderContext->SetStencilCompareFunction(STENCILCOMPARISONFUNCTION_ALWAYS);
			pRenderContext->SetStencilPassOperation(STENCILOPERATION_REPLACE);
			pRenderContext->SetStencilFailOperation(STENCILOPERATION_KEEP);
			pRenderContext->SetStencilZFailOperation(STENCILOPERATION_KEEP);
			pRenderContext->SetStencilReferenceValue(1);
			pRenderContext->SetStencilWriteMask(0xFF);
			pRenderContext->SetStencilTestMask(0x0);
		}

		// Use cached hashes if available
		if (pVisibleHashes)
		{
			for (auto& [uHash, tColor] : *pVisibleHashes)
			{
				auto pMaterial = F::Materials.GetMaterial(uHash);
				if (!pMaterial)
					continue;

				F::Materials.SetColor(pMaterial, tColor);
				I::ModelRender->ForcedMaterialOverride(pMaterial->m_pMaterial);
				if (pMaterial->m_bInvertCull)
					pRenderContext->CullMode(MATERIAL_CULLMODE_CW);
				if (pMaterial->m_bBlockOccluded)
					pRenderContext->SetStencilZFailOperation(STENCILOPERATION_REPLACE);

				m_bRendering = true;
				pEntity->DrawModel(STUDIO_RENDER);
				m_bRendering = false;

				if (pMaterial->m_bInvertCull)
					pRenderContext->CullMode(MATERIAL_CULLMODE_CCW);
				if (pMaterial->m_bBlockOccluded)
					pRenderContext->SetStencilZFailOperation(STENCILOPERATION_KEEP);
			}
		}

		if (bTwoModel)
			pRenderContext->SetStencilEnable(false);
		break;
	}
	case ModelEnum::Occluded:
	{
		if (!bTwoModel)
		{
			if (!bOccluded)
				return;
		}
		else
		{
			pRenderContext->SetStencilEnable(true);
			pRenderContext->SetStencilCompareFunction(STENCILCOMPARISONFUNCTION_EQUAL);
			pRenderContext->SetStencilPassOperation(STENCILOPERATION_KEEP);
			pRenderContext->SetStencilFailOperation(STENCILOPERATION_KEEP);
			pRenderContext->SetStencilZFailOperation(STENCILOPERATION_KEEP);
			pRenderContext->SetStencilReferenceValue(0);
			pRenderContext->SetStencilWriteMask(0x0);
			pRenderContext->SetStencilTestMask(0xFF);
		}
		pRenderContext->DepthRange(0.f, 0.2f);

		// Use cached hashes if available
		if (pOccludedHashes)
		{
			for (auto& [uHash, tColor] : *pOccludedHashes)
			{
				auto pMaterial = F::Materials.GetMaterial(uHash);
				if (!pMaterial)
					continue;

				F::Materials.SetColor(pMaterial, tColor);
				I::ModelRender->ForcedMaterialOverride(pMaterial->m_pMaterial);
				if (pMaterial->m_bInvertCull)
					pRenderContext->CullMode(MATERIAL_CULLMODE_CW);
				if (pMaterial->m_bBlockOccluded)
					pRenderContext->SetStencilZFailOperation(STENCILOPERATION_REPLACE);

				m_bRendering = true;
				pEntity->DrawModel(STUDIO_RENDER);
				m_bRendering = false;

				if (pMaterial->m_bInvertCull)
					pRenderContext->CullMode(MATERIAL_CULLMODE_CCW);
				if (pMaterial->m_bBlockOccluded)
					pRenderContext->SetStencilZFailOperation(STENCILOPERATION_KEEP);
			}
		}

		if (bTwoModel)
			pRenderContext->SetStencilEnable(false);
		pRenderContext->DepthRange(0.f, 1.f);
	}
	}
	End();
}



void CChams::Store(CTFPlayer* pLocal)
{
	m_vEntities.clear();
	if (!pLocal || !F::Groups.GroupsActive())
		return;

	for (auto& [pEntity, pGroup] : F::Groups.GetGroup())
	{
		if (pEntity->IsDormant() || !pEntity->ShouldDraw())
			continue;

		if (pGroup->m_tChams() && !pEntity->IsWearableVM()
			&& SDK::IsOnScreen(pEntity, pEntity->IsBaseCombatWeapon() || pEntity->IsWearable()))
		{
			ChamsInfo_t tInfo;
			tInfo.m_pEntity = pEntity;
			tInfo.m_pChams = &pGroup->m_tChams;
			tInfo.m_iFlags = 0;

			// Cache material hashes
			auto& vVisible = pGroup->m_tChams.GetVisible();
			tInfo.m_vVisibleHashes.reserve(vVisible.size());
			for (auto& [sName, tColor] : vVisible)
				tInfo.m_vVisibleHashes.emplace_back(FNV1A::Hash32(sName.c_str()), tColor);

			auto& vOccluded = pGroup->m_tChams.GetOccluded();
			tInfo.m_vOccludedHashes.reserve(vOccluded.size());
			for (auto& [sName, tColor] : vOccluded)
				tInfo.m_vOccludedHashes.emplace_back(FNV1A::Hash32(sName.c_str()), tColor);

			m_vEntities.emplace_back(tInfo);
		}

		if (pEntity->IsPlayer() && pEntity != pLocal && pGroup->m_iBacktrack & BacktrackEnum::Enabled && pGroup->m_tBacktrackChams(false)
			&& (F::Backtrack.GetFakeLatency() || F::Backtrack.GetFakeInterp() > G::Lerp || F::Backtrack.GetWindow()))
		{	// backtrack
			auto pWeapon = H::Entities.GetWeapon();
			if (pWeapon && (pGroup->m_iBacktrack & BacktrackEnum::Always || G::PrimaryWeaponType != EWeaponType::PROJECTILE))
			{
				bool bShowFriendly = false, bShowEnemy = true;
				if (G::PrimaryWeaponType == EWeaponType::MELEE && SDK::AttribHookValue(0, "speed_buff_ally", pWeapon) > 0)
					bShowFriendly = true;
				else if (pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN)
					bShowFriendly = true, bShowEnemy = false;

				if (bShowEnemy && pEntity->m_iTeamNum() != pLocal->m_iTeamNum() || bShowFriendly && pEntity->m_iTeamNum() == pLocal->m_iTeamNum())
				{
					ChamsInfo_t tInfo;
					tInfo.m_pEntity = pEntity;
					tInfo.m_pChams = &pGroup->m_tBacktrackChams;
					tInfo.m_iFlags = pGroup->m_iBacktrack;

					// Cache material hashes
					auto& vVisible = pGroup->m_tBacktrackChams.GetVisible();
					tInfo.m_vVisibleHashes.reserve(vVisible.size());
					for (auto& [sName, tColor] : vVisible)
						tInfo.m_vVisibleHashes.emplace_back(FNV1A::Hash32(sName.c_str()), tColor);

					auto& vOccluded = pGroup->m_tBacktrackChams.GetOccluded();
					tInfo.m_vOccludedHashes.reserve(vOccluded.size());
					for (auto& [sName, tColor] : vOccluded)
						tInfo.m_vOccludedHashes.emplace_back(FNV1A::Hash32(sName.c_str()), tColor);

					m_vEntities.emplace_back(tInfo);
				}
			}
		}
	}

	Group_t* pGroup = nullptr;
	if (F::FakeAngle.bDrawChams && F::FakeAngle.bBonesSetup
		&& F::Groups.GetGroup(TargetsEnum::FakeAngle, pGroup) && pGroup->m_tChams(false))
	{	// fakeangle
		ChamsInfo_t tInfo;
		tInfo.m_pEntity = pLocal;
		tInfo.m_pChams = &pGroup->m_tChams;
		tInfo.m_iFlags = 1;

		// Cache material hashes
		auto& vVisible = pGroup->m_tChams.GetVisible();
		tInfo.m_vVisibleHashes.reserve(vVisible.size());
		for (auto& [sName, tColor] : vVisible)
			tInfo.m_vVisibleHashes.emplace_back(FNV1A::Hash32(sName.c_str()), tColor);

		auto& vOccluded = pGroup->m_tChams.GetOccluded();
		tInfo.m_vOccludedHashes.reserve(vOccluded.size());
		for (auto& [sName, tColor] : vOccluded)
			tInfo.m_vOccludedHashes.emplace_back(FNV1A::Hash32(sName.c_str()), tColor);

		m_vEntities.emplace_back(tInfo);
	}
}

void CChams::RenderMain()
{
	auto pRenderContext = I::MaterialSystem->GetRenderContext();
	if (!pRenderContext)
		return;

	m_mEntities.clear();
	if (m_vEntities.empty())
		return;

	pRenderContext->ClearBuffers(false, false, true);

	for (auto& tInfo : m_vEntities)
	{
		if (!tInfo.m_iFlags)
			DrawModel(tInfo.m_pEntity, *tInfo.m_pChams, pRenderContext, ModelEnum::Visible, true, &tInfo.m_vVisibleHashes, &tInfo.m_vOccludedHashes);
		else
		{
			m_iFlags = tInfo.m_iFlags;

			auto pPlayer = tInfo.m_pEntity->As<CTFPlayer>();
			const float flOldInvisibility = pPlayer->m_flInvisibility();
			pPlayer->m_flInvisibility() = 0.f;
			DrawModel(tInfo.m_pEntity, *tInfo.m_pChams, pRenderContext, ModelEnum::Visible, true, &tInfo.m_vVisibleHashes, &tInfo.m_vOccludedHashes);
			pPlayer->m_flInvisibility() = flOldInvisibility;

			m_iFlags = 0;
		}
	}
	for (auto& tInfo : m_vEntities)
	{
		if (!tInfo.m_iFlags)
			DrawModel(tInfo.m_pEntity, *tInfo.m_pChams, pRenderContext, ModelEnum::Occluded, true, &tInfo.m_vVisibleHashes, &tInfo.m_vOccludedHashes);
		else
		{
			m_iFlags = tInfo.m_iFlags;

			auto pPlayer = tInfo.m_pEntity->As<CTFPlayer>();
			const float flOldInvisibility = pPlayer->m_flInvisibility();
			pPlayer->m_flInvisibility() = 0.f;
			DrawModel(tInfo.m_pEntity, *tInfo.m_pChams, pRenderContext, ModelEnum::Occluded, true, &tInfo.m_vVisibleHashes, &tInfo.m_vOccludedHashes);
			pPlayer->m_flInvisibility() = flOldInvisibility;

			m_iFlags = 0;
		}
	}

	pRenderContext->ClearBuffers(false, false, true);
}

void CChams::RenderBacktrack(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo)
{
	auto pRenderContext = I::MaterialSystem->GetRenderContext();
	if (!pRenderContext)
		return;

	auto pEntity = I::ClientEntityList->GetClientEntity(pInfo.entity_index)->As<CTFPlayer>();
	if (!pEntity || !pEntity->IsPlayer())
		return;

	std::vector<TickRecord*> vRecords = {};
	if (!F::Backtrack.GetRecords(pEntity, vRecords))
		return;
	vRecords = F::Backtrack.GetValidRecords(vRecords);
	if (!vRecords.size())
		return;

	bool bDrawLast = m_iFlags & BacktrackEnum::Last;
	bool bDrawFirst = m_iFlags & BacktrackEnum::First;

	float flOriginalBlend = I::RenderView->GetBlend();
	auto fDrawModel = [&](Vec3& vOrigin, const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo, matrix3x4* pBoneToWorld, float flBlend)
	{
		if (!SDK::IsOnScreen(pEntity, vOrigin))
			return;

		I::RenderView->SetBlend(flBlend * flOriginalBlend);
		static auto IVModelRender_DrawModelExecute = U::Hooks.m_mHooks["IVModelRender_DrawModelExecute"];
		IVModelRender_DrawModelExecute->Call<void>(I::ModelRender, pState, pInfo, pBoneToWorld);
	};
	if (!bDrawLast && !bDrawFirst)
	{
		for (auto pRecord : vRecords)
		{
			if (float flBlend = Math::RemapVal(pEntity->GetAbsOrigin().DistToSqr(pRecord->m_vOrigin), 1.f, 576.f, 0.f, 1.f))
				fDrawModel(pRecord->m_vOrigin, pState, pInfo, pRecord->m_aBones, flBlend);
		}
	}
	else
	{
		if (bDrawLast)
		{
			auto pRecord = vRecords.back();
			if (float flBlend = Math::RemapVal(pEntity->GetAbsOrigin().DistToSqr(pRecord->m_vOrigin), 1.f, 576.f, 0.f, 1.f))
				fDrawModel(pRecord->m_vOrigin, pState, pInfo, pRecord->m_aBones, flBlend);
		}
		if (bDrawFirst)
		{
			auto pRecord = vRecords.front();
			if (float flBlend = Math::RemapVal(pEntity->GetAbsOrigin().DistToSqr(pRecord->m_vOrigin), 1.f, 576.f, 0.f, 1.f))
				fDrawModel(pRecord->m_vOrigin, pState, pInfo, pRecord->m_aBones, flBlend);
		}
	}
	I::RenderView->SetBlend(flOriginalBlend);
}
void CChams::RenderFakeAngle(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo)
{
	static auto IVModelRender_DrawModelExecute = U::Hooks.m_mHooks["IVModelRender_DrawModelExecute"];
	IVModelRender_DrawModelExecute->Call<void>(I::ModelRender, pState, pInfo, F::FakeAngle.aBones);
}
void CChams::RenderHandler(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo, matrix3x4* pBoneToWorld)
{
	if (!m_iFlags)
	{
		static auto IVModelRender_DrawModelExecute = U::Hooks.m_mHooks["IVModelRender_DrawModelExecute"];
		IVModelRender_DrawModelExecute->Call<void>(I::ModelRender, pState, pInfo, pBoneToWorld);
	}
	else
	{
		if (pInfo.entity_index != I::EngineClient->GetLocalPlayer())
			RenderBacktrack(pState, pInfo);
		else
			RenderFakeAngle(pState, pInfo);
	}
}

bool CChams::RenderViewmodel(void* rcx, int flags, int* iReturn)
{
	if (!F::Groups.GroupsActive())
		return false;

	auto pRenderContext = I::MaterialSystem->GetRenderContext();
	if (!pRenderContext)
		return false;

	Group_t* pGroup = nullptr;
	if (!F::Groups.GetGroup(reinterpret_cast<CBaseAnimating*>(rcx)->IsValid() ? TargetsEnum::ViewmodelHands : TargetsEnum::ViewmodelWeapon, pGroup) || !pGroup->m_tChams(true))
		return false;

	Begin();
	for (auto& [sName, tColor] : pGroup->m_tChams.Visible)
	{
		auto pMaterial = F::Materials.GetMaterial(FNV1A::Hash32(sName.c_str()));

		F::Materials.SetColor(pMaterial, tColor);
		I::ModelRender->ForcedMaterialOverride(pMaterial ? pMaterial->m_pMaterial : nullptr);

		bool bFlip = pMaterial && pMaterial->m_bInvertCull ? !G::FlipViewmodels : G::FlipViewmodels;
		pRenderContext->CullMode(bFlip ? MATERIAL_CULLMODE_CW : MATERIAL_CULLMODE_CCW);

		static auto CBaseAnimating_InternalDrawModel = U::Hooks.m_mHooks["CBaseAnimating_InternalDrawModel"];
		*iReturn = CBaseAnimating_InternalDrawModel->Call<int>(rcx, flags);
	}
	pRenderContext->CullMode(G::FlipViewmodels ? MATERIAL_CULLMODE_CW : MATERIAL_CULLMODE_CCW);
	End();

	return true;
}
bool CChams::RenderViewmodel(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo, matrix3x4* pBoneToWorld)
{
	if (!F::Groups.GroupsActive())
		return false;

	auto pRenderContext = I::MaterialSystem->GetRenderContext();
	if (!pRenderContext)
		return false;

	Group_t* pGroup = nullptr;
	if (!F::Groups.GetGroup(TargetsEnum::ViewmodelWeapon, pGroup) || !pGroup->m_tChams(true))
		return false;

	Begin();
	for (auto& [sName, tColor] : pGroup->m_tChams.Visible)
	{
		auto pMaterial = F::Materials.GetMaterial(FNV1A::Hash32(sName.c_str()));

		F::Materials.SetColor(pMaterial, tColor);
		I::ModelRender->ForcedMaterialOverride(pMaterial ? pMaterial->m_pMaterial : nullptr);

		bool bFlip = pMaterial && pMaterial->m_bInvertCull ? !G::FlipViewmodels : G::FlipViewmodels;
		pRenderContext->CullMode(bFlip ? MATERIAL_CULLMODE_CW : MATERIAL_CULLMODE_CCW);

		static auto IVModelRender_DrawModelExecute = U::Hooks.m_mHooks["IVModelRender_DrawModelExecute"];
		IVModelRender_DrawModelExecute->Call<void>(I::ModelRender, pState, pInfo, pBoneToWorld);
	}
	pRenderContext->CullMode(MATERIAL_CULLMODE_CCW);
	End();

	return true;
}