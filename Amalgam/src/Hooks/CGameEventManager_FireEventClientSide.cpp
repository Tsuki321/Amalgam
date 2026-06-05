#include "../SDK/SDK.h"

#include "../Features/Killstreak/Killstreak.h"

MAKE_HOOK(CGameEventManager_FireEventClientSide, U::Memory.GetVirtual(I::GameEventManager, 8), bool,
	IGameEventManager2* rcx, IGameEvent* event)
{
	DEBUG_RETURN(CGameEventManager_FireEventClientSide, rcx, event);

	if (Vars::Visuals::UI::KillstreakWeapons.Value)
	{
		auto uHash = FNV1A::Hash32(event->GetName());
		if (uHash == FNV1A::Hash32Const("player_death"))
			F::Killstreak.PlayerDeath(event);
	}

	return CALL_ORIGINAL(rcx, event);
}
