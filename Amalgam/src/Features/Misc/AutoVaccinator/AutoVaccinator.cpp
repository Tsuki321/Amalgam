#include "AutoVaccinator.h"

bool CAutoVaccinator::IsUsingVaccinator(CTFPlayer* pLocal)
{
    // Check if the player is a medic
    if (pLocal->m_iClass() != TF_CLASS_MEDIC)
        return false;

    // Get the medigun (secondary weapon)
    auto pWeapon = pLocal->GetWeaponFromSlot(SLOT_SECONDARY);
    if (!pWeapon || pWeapon->GetClassID() != ETFClassID::CWeaponMedigun)
        return false;

    // Check if it's a vaccinator
    auto pMedigun = pWeapon->As<CWeaponMedigun>();
    return pMedigun->GetMedigunType() == MEDIGUN_RESIST;
}

bool CAutoVaccinator::ScanForProjectiles(CTFPlayer* pLocal, medigun_resist_types_t& bestResistType)
{
    if (!Vars::Misc::AutoVaccinator::Enabled.Value)
        return false;
        
    bool foundThreat = false;
    
    // Scan for projectiles
    for (auto pEntity : H::Entities.GetGroup(EGroupType::WORLD_PROJECTILES))
    {
        // Skip projectiles from teammates
        CBaseEntity* pOwner = nullptr;
        
        // Determine projectile owner based on class ID
        switch (pEntity->GetClassID())
        {
        case ETFClassID::CBaseProjectile:
        case ETFClassID::CBaseGrenade:
        case ETFClassID::CTFWeaponBaseGrenadeProj:
        case ETFClassID::CTFWeaponBaseMerasmusGrenade:
        case ETFClassID::CTFGrenadePipebombProjectile:
        case ETFClassID::CTFStunBall:
        case ETFClassID::CTFBall_Ornament:
        case ETFClassID::CTFProjectile_Jar:
        case ETFClassID::CTFProjectile_Cleaver:
        case ETFClassID::CTFProjectile_JarGas:
        case ETFClassID::CTFProjectile_JarMilk:
        case ETFClassID::CTFProjectile_SpellBats:
        case ETFClassID::CTFProjectile_SpellKartBats:
        case ETFClassID::CTFProjectile_SpellMeteorShower:
        case ETFClassID::CTFProjectile_SpellMirv:
        case ETFClassID::CTFProjectile_SpellPumpkin:
        case ETFClassID::CTFProjectile_SpellSpawnBoss:
        case ETFClassID::CTFProjectile_SpellSpawnHorde:
        case ETFClassID::CTFProjectile_SpellSpawnZombie:
        case ETFClassID::CTFProjectile_SpellTransposeTeleport:
        case ETFClassID::CTFProjectile_Throwable:
        case ETFClassID::CTFProjectile_ThrowableBreadMonster:
        case ETFClassID::CTFProjectile_ThrowableBrick:
        case ETFClassID::CTFProjectile_ThrowableRepel:
        {
            pOwner = pEntity->As<CTFWeaponBaseGrenadeProj>()->m_hThrower().Get();
            break;
        }
        case ETFClassID::CTFBaseRocket:
        case ETFClassID::CTFFlameRocket:
        case ETFClassID::CTFProjectile_Arrow:
        case ETFClassID::CTFProjectile_GrapplingHook:
        case ETFClassID::CTFProjectile_HealingBolt:
        case ETFClassID::CTFProjectile_Rocket:
        case ETFClassID::CTFProjectile_BallOfFire:
        case ETFClassID::CTFProjectile_MechanicalArmOrb:
        case ETFClassID::CTFProjectile_SentryRocket:
        case ETFClassID::CTFProjectile_SpellFireball:
        default:
            pOwner = pEntity->As<CTFBaseRocket>()->m_hOwnerEntity().Get();
            break;
        }

        // Skip if no owner or owner is on the same team
        if (!pOwner || pOwner->m_iTeamNum() == pLocal->m_iTeamNum())
            continue;
            
        // Check if projectile is close enough to be a threat
        Vec3 vLocalPos = pLocal->GetCenter();
        Vec3 vProjectilePos = pEntity->GetCenter();
        Vec3 vProjectileVel = pEntity->GetAbsVelocity();
        
        float flDistance = vLocalPos.DistTo(vProjectilePos);
        if (flDistance > Vars::Misc::AutoVaccinator::ProjectileRange.Value)  // Only consider threats within configured distance
            continue;
            
        // Determine projectile type and update best resistance if needed
        medigun_resist_types_t projectileResistType = MEDIGUN_BLAST_RESIST; // Default to blast resistance
        bool isCritical = false;
        
        // Check if the projectile is critical
        switch (pEntity->GetClassID())
        {
        case ETFClassID::CTFWeaponBaseGrenadeProj:
        case ETFClassID::CTFWeaponBaseMerasmusGrenade:
        case ETFClassID::CTFGrenadePipebombProjectile:
        case ETFClassID::CTFStunBall:
        case ETFClassID::CTFBall_Ornament:
        case ETFClassID::CTFProjectile_Jar:
        case ETFClassID::CTFProjectile_Cleaver:
        case ETFClassID::CTFProjectile_JarGas:
        case ETFClassID::CTFProjectile_JarMilk:
            isCritical = pEntity->As<CTFWeaponBaseGrenadeProj>()->m_bCritical();
            break;
        case ETFClassID::CTFProjectile_Rocket:
            isCritical = pEntity->As<CTFProjectile_Rocket>()->m_bCritical();
            break;
        case ETFClassID::CTFProjectile_Flare:
            isCritical = pEntity->As<CTFProjectile_Flare>()->m_bCritical();
            break;
        case ETFClassID::CTFProjectile_Arrow:
            isCritical = pEntity->As<CTFProjectile_Arrow>()->m_bCritical();
            break;
        default:
            // Assume not critical for other types
            isCritical = false;
            break;
        }
        
        // Determine the resistance type based on projectile class
        switch (pEntity->GetClassID())
        {
        case ETFClassID::CTFProjectile_Rocket:
        case ETFClassID::CTFBaseRocket:
        case ETFClassID::CTFProjectile_SentryRocket:
        case ETFClassID::CTFGrenadePipebombProjectile:
            projectileResistType = MEDIGUN_BLAST_RESIST;
            break;
        case ETFClassID::CTFProjectile_Arrow:
        case ETFClassID::CTFProjectile_HealingBolt:
        case ETFClassID::CTFProjectile_GrapplingHook:
            projectileResistType = MEDIGUN_BULLET_RESIST;
            break;
        case ETFClassID::CTFFlameRocket:
        case ETFClassID::CTFProjectile_BallOfFire:
        case ETFClassID::CTFProjectile_Flare:
        case ETFClassID::CTFProjectile_SpellFireball:
            projectileResistType = MEDIGUN_FIRE_RESIST;
            break;
        default:
            projectileResistType = MEDIGUN_BLAST_RESIST;
            break;
        }
        
        // If we prefer crit protection, check if this is a critical projectile
        if (Vars::Misc::AutoVaccinator::PreferCrit.Value && isCritical)
        {
            bestResistType = projectileResistType;
            return true;
        }
        
        // Otherwise, just set the resistance type and mark that we found a threat
        bestResistType = projectileResistType;
        foundThreat = true;
    }
    
    return foundThreat;
}

medigun_resist_types_t CAutoVaccinator::GetBestResistType(CTFPlayer* pLocal)
{
    // Default to blast resistance
    medigun_resist_types_t bestResistType = MEDIGUN_BLAST_RESIST;
    
    // Scan for projectiles first
    if (ScanForProjectiles(pLocal, bestResistType))
        return bestResistType;
    
    // Check for enemy players and their weapons
    int bulletThreats = 0;
    int blastThreats = 0;
    int fireThreats = 0;
    
    for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
    {
        auto pPlayer = pEntity->As<CTFPlayer>();
        if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsAGhost() || pPlayer->IsDormant())
            continue;
        
        // Calculate distance and check if they're close enough to be a threat
        float flDistance = pLocal->GetCenter().DistTo(pPlayer->GetCenter());
        if (flDistance > Vars::Misc::AutoVaccinator::PlayerRange.Value)  // Only consider threats within configured distance
            continue;
        
        // Check their active weapon
        auto pWeapon = pPlayer->m_hActiveWeapon().Get();
        if (!pWeapon)
            continue;
        
        // Determine weapon type threat
        switch (pWeapon->GetWeaponID())
        {
        // Hitscan weapons (bullet resistance)
        case TF_WEAPON_SCATTERGUN:
        case TF_WEAPON_SHOTGUN_PRIMARY:
        case TF_WEAPON_SHOTGUN_SOLDIER:
        case TF_WEAPON_SHOTGUN_HWG:
        case TF_WEAPON_SHOTGUN_PYRO:
        case TF_WEAPON_PISTOL:
        case TF_WEAPON_PISTOL_SCOUT:
        case TF_WEAPON_REVOLVER:
        case TF_WEAPON_MINIGUN:
        case TF_WEAPON_SMG:
        case TF_WEAPON_SNIPERRIFLE:
        case TF_WEAPON_COMPOUND_BOW:  // Huntsman is both bullet and projectile
            bulletThreats += pPlayer->IsCritBoosted() ? 3 : 1;
            break;
            
        // Explosive weapons (blast resistance)
        case TF_WEAPON_ROCKETLAUNCHER:
        case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
        case TF_WEAPON_GRENADELAUNCHER:
        case TF_WEAPON_PIPEBOMBLAUNCHER:
        case TF_WEAPON_CANNON:
            blastThreats += pPlayer->IsCritBoosted() ? 3 : 1;
            break;
            
        // Fire weapons (fire resistance)
        case TF_WEAPON_FLAMETHROWER:
        case TF_WEAPON_FLAME_BALL:
        case TF_WEAPON_FLAREGUN:
        case TF_WEAPON_FLAREGUN_REVENGE:
            fireThreats += pPlayer->IsCritBoosted() ? 3 : 1;
            break;
            
        // For other weapons, count as bullet resistance by default
        default:
            bulletThreats++;
            break;
        }
    }
    
    // Determine the best resistance type based on threat counts
    if (bulletThreats >= blastThreats && bulletThreats >= fireThreats)
        return MEDIGUN_BULLET_RESIST;
    else if (blastThreats >= bulletThreats && blastThreats >= fireThreats)
        return MEDIGUN_BLAST_RESIST;
    else
        return MEDIGUN_FIRE_RESIST;
}

bool CAutoVaccinator::ShouldPopcritUber(CTFPlayer* pLocal, medigun_resist_types_t resistType)
{
    if (!Vars::Misc::AutoVaccinator::Enabled.Value || !Vars::Misc::AutoVaccinator::PopUber.Value)
        return false;
        
    // Get the medigun
    auto pWeapon = pLocal->GetWeaponFromSlot(SLOT_SECONDARY);
    if (!pWeapon || pWeapon->GetClassID() != ETFClassID::CWeaponMedigun)
        return false;
        
    auto pMedigun = pWeapon->As<CWeaponMedigun>();
    
    // Check if we have enough uber to pop
    if (pMedigun->m_flChargeLevel() < 0.25f)  // Need at least one charge segment
        return false;
    
    // Scan for critical threats specifically for the given resistance type
    for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
    {
        auto pPlayer = pEntity->As<CTFPlayer>();
        if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsAGhost() || pPlayer->IsDormant())
            continue;
        
        // Check if they're in range and visible
        float flDistance = pLocal->GetCenter().DistTo(pPlayer->GetCenter());
        if (flDistance > Vars::Misc::AutoVaccinator::PlayerRange.Value || !SDK::VisPos(pLocal, pPlayer, pLocal->GetCenter(), pPlayer->GetCenter()))
            continue;
            
        // Check if they're crit boosted and using a weapon that matches our resistance type
        if (pPlayer->IsCritBoosted() || pPlayer->InCond(TF_COND_CRITBOOSTED) || 
            pPlayer->InCond(TF_COND_CRITBOOSTED_PUMPKIN) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_USER_BUFF) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_DEMO_CHARGE) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_FIRST_BLOOD) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_BONUS_TIME) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_CTF_CAPTURE) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_ON_KILL) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_RAGE_BUFF) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_CARD_EFFECT) ||
            pPlayer->InCond(TF_COND_CRITBOOSTED_RUNE_TEMP))
        {
            auto pWeapon = pPlayer->m_hActiveWeapon().Get();
            if (!pWeapon)
                continue;
                
            bool shouldPop = false;
            
            switch (resistType)
            {
            case MEDIGUN_BULLET_RESIST:
                // Pop uber for bullet weapons
                shouldPop = (pWeapon->GetWeaponID() == TF_WEAPON_SCATTERGUN || 
                           pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PRIMARY ||
                           pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_SOLDIER ||
                           pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_HWG ||
                           pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PYRO ||
                           pWeapon->GetWeaponID() == TF_WEAPON_PISTOL ||
                           pWeapon->GetWeaponID() == TF_WEAPON_PISTOL_SCOUT ||
                           pWeapon->GetWeaponID() == TF_WEAPON_REVOLVER ||
                           pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN ||
                           pWeapon->GetWeaponID() == TF_WEAPON_SMG ||
                           pWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE ||
                           pWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW);
                break;
                
            case MEDIGUN_BLAST_RESIST:
                // Pop uber for explosive weapons
                shouldPop = (pWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER ||
                           pWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT ||
                           pWeapon->GetWeaponID() == TF_WEAPON_GRENADELAUNCHER ||
                           pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER ||
                           pWeapon->GetWeaponID() == TF_WEAPON_CANNON);
                break;
                
            case MEDIGUN_FIRE_RESIST:
                // Pop uber for fire weapons
                shouldPop = (pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER ||
                           pWeapon->GetWeaponID() == TF_WEAPON_FLAME_BALL ||
                           pWeapon->GetWeaponID() == TF_WEAPON_FLAREGUN ||
                           pWeapon->GetWeaponID() == TF_WEAPON_FLAREGUN_REVENGE);
                break;
            }
            
            if (shouldPop)
                return true;
        }
    }
    
    // Also scan for critical projectiles
    for (auto pEntity : H::Entities.GetGroup(EGroupType::WORLD_PROJECTILES))
    {
        // Check if projectile is close enough and critical
        bool isCritical = false;
        medigun_resist_types_t projectileResistType = MEDIGUN_BLAST_RESIST;
        
        // Determine if it's critical based on class ID
        switch (pEntity->GetClassID())
        {
        case ETFClassID::CTFWeaponBaseGrenadeProj:
        case ETFClassID::CTFWeaponBaseMerasmusGrenade:
        case ETFClassID::CTFGrenadePipebombProjectile:
        case ETFClassID::CTFStunBall:
        case ETFClassID::CTFBall_Ornament:
        case ETFClassID::CTFProjectile_Jar:
        case ETFClassID::CTFProjectile_Cleaver:
        case ETFClassID::CTFProjectile_JarGas:
        case ETFClassID::CTFProjectile_JarMilk:
            isCritical = pEntity->As<CTFWeaponBaseGrenadeProj>()->m_bCritical();
            break;
        case ETFClassID::CTFProjectile_Rocket:
            isCritical = pEntity->As<CTFProjectile_Rocket>()->m_bCritical();
            break;
        case ETFClassID::CTFProjectile_Flare:
            isCritical = pEntity->As<CTFProjectile_Flare>()->m_bCritical();
            break;
        case ETFClassID::CTFProjectile_Arrow:
            isCritical = pEntity->As<CTFProjectile_Arrow>()->m_bCritical();
            break;
        default:
            // Assume not critical for other types
            isCritical = false;
            break;
        }
        
        if (!isCritical)
            continue;
            
        // Determine the resistance type based on projectile class
        switch (pEntity->GetClassID())
        {
        case ETFClassID::CTFProjectile_Rocket:
        case ETFClassID::CTFBaseRocket:
        case ETFClassID::CTFProjectile_SentryRocket:
        case ETFClassID::CTFGrenadePipebombProjectile:
            projectileResistType = MEDIGUN_BLAST_RESIST;
            break;
        case ETFClassID::CTFProjectile_Arrow:
        case ETFClassID::CTFProjectile_HealingBolt:
        case ETFClassID::CTFProjectile_GrapplingHook:
            projectileResistType = MEDIGUN_BULLET_RESIST;
            break;
        case ETFClassID::CTFFlameRocket:
        case ETFClassID::CTFProjectile_BallOfFire:
        case ETFClassID::CTFProjectile_Flare:
        case ETFClassID::CTFProjectile_SpellFireball:
            projectileResistType = MEDIGUN_FIRE_RESIST;
            break;
        default:
            // For any other projectile, use blast resistance as default
            projectileResistType = MEDIGUN_BLAST_RESIST;
            break;
        }
        
        // If the projectile type matches our resistance type and it's a crit, pop uber
        if (projectileResistType == resistType)
        {
            // Check if it's close enough
            Vec3 vLocalPos = pLocal->GetCenter();
            Vec3 vProjectilePos = pEntity->GetCenter();
            
            float flDistance = vLocalPos.DistTo(vProjectilePos);
            if (flDistance < 300.0f)  // Only pop for close enough threats
                return true;
        }
    }
    
    // If we have more than 75% uber and there are enemies nearby, use it on regular attacks
    if (pMedigun->m_flChargeLevel() > 0.75f)
    {
        for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
        {
            auto pPlayer = pEntity->As<CTFPlayer>();
            if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsAGhost() || pPlayer->IsDormant())
                continue;
            
            // Check if they're attacking and in range
            float flDistance = pLocal->GetCenter().DistTo(pPlayer->GetCenter());
            if (flDistance < 400.0f && pPlayer->m_hActiveWeapon().Get())
            {
                auto pWeapon = pPlayer->m_hActiveWeapon().Get();
                bool isRelevantWeapon = false;
                
                switch (resistType)
                {
                case MEDIGUN_BULLET_RESIST:
                    isRelevantWeapon = (pWeapon->GetWeaponID() == TF_WEAPON_SCATTERGUN || 
                                      pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PRIMARY ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_SOLDIER ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_HWG ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PYRO ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_PISTOL ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_PISTOL_SCOUT ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_REVOLVER ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_SMG ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW);
                    break;
                    
                case MEDIGUN_BLAST_RESIST:
                    isRelevantWeapon = (pWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_GRENADELAUNCHER ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_CANNON);
                    break;
                    
                case MEDIGUN_FIRE_RESIST:
                    isRelevantWeapon = (pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_FLAME_BALL ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_FLAREGUN ||
                                      pWeapon->GetWeaponID() == TF_WEAPON_FLAREGUN_REVENGE);
                    break;
                }
                
                if (isRelevantWeapon)
                    return true;
            }
        }
    }
    
    return false;
}

void CAutoVaccinator::Run(CTFPlayer* pLocal, CUserCmd* pCmd)
{
    if (!pLocal || !pLocal->IsAlive() || !IsUsingVaccinator(pLocal) || !Vars::Misc::AutoVaccinator::Enabled.Value)
        return;
    
    // Get the medigun
    auto pWeapon = pLocal->GetWeaponFromSlot(SLOT_SECONDARY);
    if (!pWeapon || pWeapon->GetClassID() != ETFClassID::CWeaponMedigun)
        return;
        
    auto pMedigun = pWeapon->As<CWeaponMedigun>();
    
    // Get the current and best resistance types
    medigun_resist_types_t currentResistType = pMedigun->GetResistType();
    medigun_resist_types_t bestResistType = GetBestResistType(pLocal);
    
    // Change resistance if needed
    if (currentResistType != bestResistType)
    {
        pCmd->buttons |= IN_RELOAD;
    }
    
    // Check if we should pop uber
    if (ShouldPopcritUber(pLocal, currentResistType))
    {
        pCmd->buttons |= IN_ATTACK2;
    }
} 