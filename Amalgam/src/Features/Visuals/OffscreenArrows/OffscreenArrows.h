#pragma once
#include "../../../SDK/SDK.h"

struct ArrowCache_t
{
	CBaseEntity* m_pEntity = nullptr;
	Color_t m_tColor = {};
	int m_iOffset = 0;
	float m_flMaxDistance = 0.f;
};

class COffscreenArrows
{
private:
	void DrawArrowTo(const Vec3& vFromPos, const Vec3& vToPos, Color_t tColor, int iOffset, float flMaxDistance);

	std::vector<ArrowCache_t> m_vCache = {};

public:
	void Store();
	void Draw(CTFPlayer* pLocal);
};

ADD_FEATURE(COffscreenArrows, OffscreenArrows);
