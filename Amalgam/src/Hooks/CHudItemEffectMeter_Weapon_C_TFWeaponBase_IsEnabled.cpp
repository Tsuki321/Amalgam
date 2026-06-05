#include "../SDK/SDK.h"

MAKE_SIGNATURE(CHudItemEffectMeter_Weapon_C_TFWeaponBase_IsEnabled, "client.dll", "48 83 EC ? E8 ? ? ? ? 48 85 C0 74 ? 45 33 C9 C6 44 24 ? ? 4C 8B C0 48 8D 15 ? ? ? ? 33 C9 E8 ? ? ? ? 85 C0 0F 95 C0 48 83 C4 ? C3", 0x0);

MAKE_HOOK(CHudItemEffectMeter_Weapon_C_TFWeaponBase_IsEnabled, S::CHudItemEffectMeter_Weapon_C_TFWeaponBase_IsEnabled(), bool)
{
	DEBUG_RETURN(CHudItemEffectMeter_Weapon_C_TFWeaponBase_IsEnabled);

	return Vars::Visuals::UI::KillstreakWeapons.Value ? true : CALL_ORIGINAL();
}
