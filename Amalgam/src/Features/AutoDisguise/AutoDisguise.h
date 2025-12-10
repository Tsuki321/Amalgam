#pragma once
#include "../../SDK/SDK.h"

class CAutoDisguise
{
public:
	void Event(IGameEvent* pEvent, uint32_t uHash, CTFPlayer* pLocal);
};

ADD_FEATURE(CAutoDisguise, AutoDisguise);