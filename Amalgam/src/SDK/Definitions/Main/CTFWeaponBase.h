#pragma once
#include "CBaseCombatWeapon.h"

MAKE_SIGNATURE(CTFWeaponBase_GetSpreadAngles, "client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F 29 74 24 ? 48 8B DA 48 8B F9 E8 ? ? ? ? 48 8B C8", 0x0);
MAKE_SIGNATURE(CTFWeaponBase_UpdateAllViewmodelAddons, "client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B F8", 0x0);
MAKE_SIGNATURE(CTFWeaponBase_CalcIsAttackCritical, "client.dll", "48 89 74 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B C8 C7 44 24 ? ? ? ? ? 4C 8D 0D ? ? ? ? 33 D2 4C 8D 05 ? ? ? ? E8 ? ? ? ? 48 8B F0 48 85 C0 0F 84 ? ? ? ? 48 8B 10", 0x0);
MAKE_SIGNATURE(CTFWeaponBaseMelee_CalcIsAttackCriticalHelper, "client.dll", "40 57 48 83 EC ? 48 8B 05 ? ? ? ? 48 8B F9 83 78 ? ? 75", 0x0);
MAKE_SIGNATURE(CTFWeaponBase_CalcIsAttackCriticalHelper, "client.dll", "48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 0F 29 74 24", 0x0);
MAKE_SIGNATURE(CTFWeaponBase_GetAppropriateWorldOrViewModel, "client.dll", "48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B C8 C7 44 24 ? ? ? ? ? 4C 8D 0D ? ? ? ? 33 D2 4C 8D 05 ? ? ? ? E8 ? ? ? ? 48 8B F8 48 85 C0 74 ? 48 8B CB", 0x0);
MAKE_SIGNATURE(CTFWeaponBase_IncrementAmmo, "client.dll", "48 89 5C 24 ? 56 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B C8", 0x0);
MAKE_SIGNATURE(CTFWeaponBase_GetMaxClip1, "client.dll", "40 53 48 83 EC ? 48 8B 01 48 8B D9 FF 90 ? ? ? ? 48 8B CB 84 C0 74", 0x0);

class CTFPlayer;
class CTFGrenadePipebombProjectile;

//credits: KGB (all weapon info stuff below)
typedef unsigned short WEAPON_FILE_INFO_HANDLE;

typedef enum
{
	EMPTY,
	SINGLE,
	SINGLE_NPC,
	WPN_DOUBLE,
	DOUBLE_NPC,
	BURST,
	RELOAD,
	RELOAD_NPC,
	MELEE_MISS,
	MELEE_HIT,
	MELEE_HIT_WORLD,
	SPECIAL1,
	SPECIAL2,
	SPECIAL3,
	TAUNT,
	DEPLOY,
	NUM_SHOOT_SOUND_TYPES,
} WeaponSound_t;

#define MAX_SHOOT_SOUNDS 16
#define MAX_WEAPON_STRING 80
#define MAX_WEAPON_PREFIX 16
#define MAX_WEAPON_AMMO_NAME 32

class FileWeaponInfo_t
{
public:
	void* m_pVtable;

	bool bParsedScript;
	bool bLoadedHudElements;

	// SHARED
	char szClassName[MAX_WEAPON_STRING];
	char szPrintName[MAX_WEAPON_STRING]; // Name for showing in HUD, etc.

	char szViewModel[MAX_WEAPON_STRING];       // View model of this weapon
	char szWorldModel[MAX_WEAPON_STRING];      // Model of this weapon seen carried by the player
	char szAnimationPrefix[MAX_WEAPON_PREFIX]; // Prefix of the animations that should be used by the player carrying this weapon
	int  iSlot;                                // inventory slot.
	int  iPosition;                            // position in the inventory slot.
	int  iMaxClip1;                            // max primary clip size (-1 if no clip)
	int  iMaxClip2;                            // max secondary clip size (-1 if no clip)
	int  iDefaultClip1;                        // amount of primary ammo in the gun when it's created
	int  iDefaultClip2;                        // amount of secondary ammo in the gun when it's created
	int  iWeight;                              // this value used to determine this weapon's importance in autoselection.
	int  iRumbleEffect;                        // Which rumble effect to use when fired? (xbox)
	bool bAutoSwitchTo;                        // whether this weapon should be considered for autoswitching to
	bool bAutoSwitchFrom;                      // whether this weapon can be autoswitched away from when picking up another weapon or ammo
	int  iFlags;                               // miscellaneous weapon flags
	char szAmmo1[MAX_WEAPON_AMMO_NAME];        // "primary" ammo type
	char szAmmo2[MAX_WEAPON_AMMO_NAME];        // "secondary" ammo type

	// Sound blocks
	char aShootSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];

	int  iAmmoType;
	int  iAmmo2Type;
	bool m_bMeleeWeapon; // Melee weapons can always "fire" regardless of ammo.

	// This tells if the weapon was built right-handed (defaults to true).
	// This helps cl_righthand make the decision about whether to flip the model or not.
	bool m_bBuiltRightHanded;
	bool m_bAllowFlipping; // False to disallow flipping the model, regardless of whether
	// it is built left or right handed.

	// CLIENT DLL
	// Sprite data, read from the data file
	int   iSpriteCount;
	CHudTexture* iconActive;
	CHudTexture* iconInactive;
	CHudTexture* iconAmmo;
	CHudTexture* iconAmmo2;
	CHudTexture* iconCrosshair;
	CHudTexture* iconAutoaim;
	CHudTexture* iconZoomedCrosshair;
	CHudTexture* iconZoomedAutoaim;
	CHudTexture* iconSmall;

	// TF2 specific
	bool bShowUsageHint; // if true, then when you receive the weapon, show a hint about it
};

struct WeaponData_t
{
	int   m_nDamage;
	int   m_nBulletsPerShot;
	float m_flRange;
	float m_flSpread;
	float m_flPunchAngle;
	float m_flTimeFireDelay;   // Time to delay between firing
	float m_flTimeIdle;        // Time to idle after firing
	float m_flTimeIdleEmpty;   // Time to idle after firing last bullet in clip
	float m_flTimeReloadStart; // Time to start into a reload (ie. shotgun)
	float m_flTimeReload;      // Time to reload
	bool  m_bDrawCrosshair;    // Should the weapon draw a crosshair
	int   m_iProjectile;       // The type of projectile this mode fires
	int   m_iAmmoPerShot;      // How much ammo each shot consumes
	float m_flProjectileSpeed; // Start speed for projectiles (nail, etc.); NOTE: union with something non-projectile
	float m_flSmackDelay;      // how long after swing should damage happen for melee weapons
	bool  m_bUseRapidFireCrits;

	void Init()
	{
		m_nDamage = 0;
		m_nBulletsPerShot = 0;
		m_flRange = 0.f;
		m_flSpread = 0.f;
		m_flPunchAngle = 0.f;
		m_flTimeFireDelay = 0.f;
		m_flTimeIdle = 0.f;
		m_flTimeIdleEmpty = 0.f;
		m_flTimeReloadStart = 0.f;
		m_flTimeReload = 0.f;
		m_iProjectile = 0;
		m_iAmmoPerShot = 0;
		m_flProjectileSpeed = 0.f;
		m_flSmackDelay = 0.f;
		m_bUseRapidFireCrits = false;
	};
};

class CTFWeaponInfo : public FileWeaponInfo_t
{
public:
	WeaponData_t m_WeaponData[2];                    //0x06F8
	int          m_iWeaponType;                      //0x0778
	int          m_bGrenade;                         //0x077C
	float        m_flDamageRadius;                   //0x0780
	float        m_flPrimerTime;                     //0x0784
	bool         m_bLowerWeapon;                     //0x0788
	bool         m_bSuppressGrenTimer;               //0x0789
	bool         m_bHasTeamSkins_Viewmodel;          //0x078A
	bool         m_bHasTeamSkins_Worldmodel;         //0x078B
	char         m_szMuzzleFlashModel[128];          //0x078C
	float        m_flMuzzleFlashModelDuration;       //0x080C
	char         m_szMuzzleFlashParticleEffect[128]; //0x0810
	char         m_szTracerEffect[128];              //0x0890
	bool         m_bDoInstantEjectBrass;             //0x0910
	char         m_szBrassModel[128];                //0x0911
	char         m_szExplosionSound[128];            //0x0991
	char         m_szExplosionEffect[128];           //0x0A11
	char         m_szExplosionPlayerEffect[128];     //0x0A91
	char         m_szExplosionWaterEffect[128];      //0x0B11
	bool         m_bDontDrop;                        //0x0B91

	WeaponData_t const& GetWeaponData(int iWeapon) const { return m_WeaponData[iWeapon]; }
};

class CTFWeaponBase : public CBaseCombatWeapon
{
public:
	NETVAR(m_bLowered, bool, "CTFWeaponBase", "m_bLowered");
	NETVAR(m_iReloadMode, int, "CTFWeaponBase", "m_iReloadMode");
	NETVAR(m_bResetParity, bool, "CTFWeaponBase", "m_bResetParity");
	NETVAR(m_bReloadedThroughAnimEvent, bool, "CTFWeaponBase", "m_bReloadedThroughAnimEvent");
	NETVAR(m_bDisguiseWeapon, bool, "CTFWeaponBase", "m_bDisguiseWeapon");
	NETVAR(m_flLastCritCheckTime, float, "CTFWeaponBase", "m_flLastCritCheckTime");
	NETVAR(m_flReloadPriorNextFire, float, "CTFWeaponBase", "m_flReloadPriorNextFire");
	NETVAR(m_flLastFireTime, float, "CTFWeaponBase", "m_flLastFireTime");
	NETVAR(m_flEffectBarRegenTime, float, "CTFWeaponBase", "m_flEffectBarRegenTime");
	NETVAR(m_flObservedCritChance, float, "CTFWeaponBase", "m_flObservedCritChance");
	NETVAR(m_flEnergy, float, "CTFWeaponBase", "m_flEnergy");
	NETVAR(m_hExtraWearable, EHANDLE, "CTFWeaponBase", "m_hExtraWearable");
	NETVAR(m_hExtraWearableViewModel, EHANDLE, "CTFWeaponBase", "m_hExtraWearableViewModel");
	NETVAR(m_bBeingRepurposedForTaunt, bool, "CTFWeaponBase", "m_bBeingRepurposedForTaunt");
	NETVAR(m_nKillComboClass, int, "CTFWeaponBase", "m_nKillComboClass");
	NETVAR(m_nKillComboCount, int, "CTFWeaponBase", "m_nKillComboCount");
	NETVAR(m_flInspectAnimEndTime, float, "CTFWeaponBase", "m_flInspectAnimEndTime");
	NETVAR(m_nInspectStage, int, "CTFWeaponBase", "m_nInspectStage");
	NETVAR(m_iConsecutiveShots, int, "CTFWeaponBase", "m_iConsecutiveShots");

	NETVAR_OFF(GetWeaponInfo, CTFWeaponInfo*, "CTFWeaponBase", "m_flEffectBarRegenTime", 16);
	NETVAR_OFF(m_flSmackTime, float, "CTFWeaponBase", "m_nInspectStage", 28);
	NETVAR_OFF(m_flCritTokenBucket, float, "CTFWeaponBase", "m_iReloadMode", -244);
	NETVAR_OFF(m_nCritChecks, int, "CTFWeaponBase", "m_iReloadMode", -240);
	NETVAR_OFF(m_nCritSeedRequests, int, "CTFWeaponBase", "m_iReloadMode", -236);
	NETVAR_OFF(m_iWeaponMode, int, "CTFWeaponBase", "m_iReloadMode", -4);
	NETVAR_OFF(m_flCritTime, float, "CTFWeaponBase", "m_flLastCritCheckTime", -4);
	NETVAR_OFF(m_iCurrentSeed, int, "CTFWeaponBase", "m_flLastCritCheckTime", 8);
	NETVAR_OFF(m_flLastRapidFireCritCheckTime, float, "CTFWeaponBase", "m_flLastCritCheckTime", 12);
	inline void* m_pMeter()
	{
		static int nOffset = U::NetVars.GetNetVar("CTFWeaponBase", "m_flEffectBarRegenTime") + -40;
		return reinterpret_cast<void*>(uintptr_t(this) + nOffset);
	};

	VIRTUAL(GetSlot, int, 331, this);
	VIRTUAL(GetWeaponID, int, 383, this);
	VIRTUAL(GetDamageType, int, 384, this);
	VIRTUAL(IsEnergyWeapon, bool, 435, this);
	VIRTUAL(AreRandomCritsEnabled, bool, 405, this);
	VIRTUAL(GetWeaponSpread, float, 470, this);
	VIRTUAL(GetSwingRange, int, 458, this);
	VIRTUAL_ARGS(ApplyFireDelay, float, 410, (float flDelay), this, flDelay);
	
	SIGNATURE(GetMaxClip1, float, CTFWeaponBase, this);
	SIGNATURE(IncrementAmmo, void, CTFWeaponBase, this);
	SIGNATURE(CalcIsAttackCritical, bool, CTFWeaponBase, this);
	SIGNATURE(CalcIsAttackCriticalHelper, bool, CTFWeaponBase, this);
	SIGNATURE(UpdateAllViewmodelAddons, bool, CTFWeaponBase, this);
	SIGNATURE(GetAppropriateWorldOrViewModel, CBaseAnimating*, CTFWeaponBase, this);
	SIGNATURE_ARGS(GetSpreadAngles, void, CTFWeaponBase, (Vec3& out), this, std::ref(out));
	Vec3 GetSpreadAngles()
	{
		Vec3 vOut;
		GetSpreadAngles(vOut);
		return vOut;
	}

	bool HasPrimaryAmmoForShot();
	bool CanPrimaryAttack();
	bool CanSecondaryAttack();
	bool CanFireCriticalShot(bool bIsHeadshot = false);
	bool IsInReload();
	bool CanHeadshot();
	bool AmbassadorCanHeadshot(float flCurTime = I::GlobalVars->curtime);
	float GetDamage(bool bAttribHookValue = true);
	float GetFireRate(bool bAttribHookValue = true);
	int GetBulletsPerShot(bool bAttribHookValue = true);
	int GetAmmoPerShot(bool bAttribHookValue = true);
	bool IsRapidFire();
	float GetRange();
	CHudTexture* GetWeaponIcon();
};

class CTFWeaponBaseMelee : public CTFWeaponBase
{
public:
	SIGNATURE(CalcIsAttackCriticalHelper, bool, CTFWeaponBaseMelee, this);
};

class CTFKnife : public CTFWeaponBase
{
public:
	NETVAR(m_bReadyToBackstab, bool, "CTFKnife", "m_bReadyToBackstab");
	NETVAR(m_bKnifeExists, bool, "CTFKnife", "m_bKnifeExists");
	NETVAR(m_flKnifeRegenerateDuration, float, "CTFKnife", "m_flKnifeRegenerateDuration");
	NETVAR(m_flKnifeMeltTimestamp, float, "CTFKnife", "m_flKnifeMeltTimestamp");
};

class CTFMinigun : public CTFWeaponBase
{
public:
	NETVAR(m_iWeaponState, int, "CTFMinigun", "m_iWeaponState");
	NETVAR(m_bCritShot, bool, "CTFMinigun", "m_bCritShot");
};

class CWeaponMedigun : public CTFWeaponBase
{
public:
	NETVAR(m_hHealingTarget, EHANDLE, "CWeaponMedigun", "m_hHealingTarget");
	NETVAR(m_bHealing, bool, "CWeaponMedigun", "m_bHealing");
	NETVAR(m_bAttacking, bool, "CWeaponMedigun", "m_bAttacking");
	NETVAR(m_bChargeRelease, bool, "CWeaponMedigun", "m_bChargeRelease");
	NETVAR(m_bHolstered, bool, "CWeaponMedigun", "m_bHolstered");
	NETVAR(m_nChargeResistType, int, "CWeaponMedigun", "m_nChargeResistType");
	NETVAR(m_hLastHealingTarget, EHANDLE, "CWeaponMedigun", "m_hLastHealingTarget");
	NETVAR(m_flChargeLevel, float, "CWeaponMedigun", "m_flChargeLevel");

	int GetMedigunType();
	MedigunChargeTypes GetChargeType();
	medigun_resist_types_t GetResistType();
};

class CTFPipebombLauncher : public CTFWeaponBase
{
public:
	NETVAR(m_iPipebombCount, int, "CTFPipebombLauncher", "m_iPipebombCount");
	NETVAR(m_flChargeBeginTime, float, "CTFPipebombLauncher", "m_flChargeBeginTime");

	NETVAR_OFF(m_Pipebombs, CUtlVector<CHandle<CTFGrenadePipebombProjectile>>, "CTFPipebombLauncher", "m_flChargeBeginTime", -28);

	int GetDetonateType();
};

class CTFSniperRifle : public CTFWeaponBase
{
public:
	NETVAR(m_flChargedDamage, float, "CTFSniperRifle", "m_flChargedDamage");

	int GetRifleType();
	float GetHeadshotMult(CTFPlayer* pTarget = nullptr);
	float GetBodyshotMult(CTFPlayer* pTarget = nullptr);
};

class CTFGrenadeLauncher : public CTFWeaponBase
{
public:
	NETVAR(m_flDetonateTime, float, "CTFGrenadeLauncher", "m_flDetonateTime");
	NETVAR(m_iCurrentTube, int, "CTFGrenadeLauncher", "m_iCurrentTube");
	NETVAR(m_iGoalTube, int, "CTFGrenadeLauncher", "m_iGoalTube");

	int GetDetonateType();
};

class CTFSniperRifleClassic : public CTFSniperRifle
{
public:
	NETVAR(m_bCharging, bool, "CTFSniperRifleClassic", "m_bCharging");
};

class CTFParticleCannon : public CTFWeaponBase
{
public:
	NETVAR(m_flChargeBeginTime, float, "CTFParticleCannon", "m_flChargeBeginTime");
	NETVAR(m_iChargeEffect, int, "CTFParticleCannon", "m_iChargeEffect");
};

class CTFFlareGun : public CTFWeaponBase
{
public:
	int GetFlareGunType();
};

class CTFThrowable : public CTFWeaponBase
{
public:
	NETVAR(m_flChargeBeginTime, float, "CTFThrowable", "m_flChargeBeginTime");
};

class CTFGrapplingHook : public CTFWeaponBase
{
public:
	NETVAR(m_hProjectile, EHANDLE, "CTFGrapplingHook", "m_hProjectile");
};

class CTFSpellBook : public CTFThrowable
{
public:
	NETVAR(m_flTimeNextSpell, float, "CTFSpellBook", "m_flTimeNextSpell");
	NETVAR(m_iSelectedSpellIndex, int, "CTFSpellBook", "m_iSelectedSpellIndex");
	NETVAR(m_iSpellCharges, int, "CTFSpellBook", "m_iSpellCharges");
	NETVAR(m_bFiredAttack, bool, "CTFSpellBook", "m_bFiredAttack");
};