#pragma once
#include "../../../SDK/SDK.h"

class CAutoVaccinator
{
private:
    bool IsUsingVaccinator(CTFPlayer* pLocal);
    medigun_resist_types_t GetBestResistType(CTFPlayer* pLocal);
    bool ShouldPopcritUber(CTFPlayer* pLocal, medigun_resist_types_t resistType);
    bool ScanForProjectiles(CTFPlayer* pLocal, medigun_resist_types_t& bestResistType);
    
public:
    void Run(CTFPlayer* pLocal, CUserCmd* pCmd);
};

ADD_FEATURE(CAutoVaccinator, AutoVaccinator) 