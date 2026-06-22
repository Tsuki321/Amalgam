#pragma once
#include "../../../SDK/SDK.h"
#include <bitset>

Enum(Model, Visible, Occluded);

class CChams
{
private:
	void Begin();
	void End();

	void DrawModel(CBaseEntity* pEntity, const Chams_t& tChams, IMatRenderContext* pRenderContext, int iModel = ModelEnum::Visible, bool bTwoModel = false, const std::vector<std::pair<uint32_t, Color_t>>* pVisibleHashes = nullptr, const std::vector<std::pair<uint32_t, Color_t>>* pOccludedHashes = nullptr);

	void RenderBacktrack(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo);
	void RenderFakeAngle(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo);

	struct ChamsInfo_t
	{
		CBaseEntity* m_pEntity;
		Chams_t* m_pChams;
		int m_iFlags = 0;
		std::vector<std::pair<uint32_t, Color_t>> m_vVisibleHashes = {};
		std::vector<std::pair<uint32_t, Color_t>> m_vOccludedHashes = {};
	};
	std::vector<ChamsInfo_t> m_vEntities = {};

	Color_t m_tOriginalColor = {};
	float m_flOriginalBlend = 1.f;
	IMaterial* m_pOriginalMaterial = nullptr;
	OverrideType_t m_iOriginalOverride = OVERRIDE_NORMAL;

	int m_iFlags = false;

public:
	void Store(CTFPlayer* pLocal);
	void RenderMain();
	void RenderHandler(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo, matrix3x4* pBoneToWorld);

	bool RenderViewmodel(void* rcx, int flags, int* iReturn);
	bool RenderViewmodel(const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo, matrix3x4* pBoneToWorld);

	bool m_bRendering = false;

	std::bitset<MAX_EDICTS> m_bEntities = {};
};

ADD_FEATURE(CChams, Chams);