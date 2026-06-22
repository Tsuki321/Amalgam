# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Amalgam is a Team Fortress 2 (TF2) game enhancement DLL written in C++. It hooks into the TF2 client to provide aimbot, visuals (ESP, chams, glow), backtracking, anti-aim, and other competitive gameplay features. The project uses hook-based architecture to intercept and modify game engine functions.

This is a fork of rei-2/Amalgam with experimental optimizations and enhancements.

## Build System

**IMPORTANT: Do not compile locally. Always use GitHub Actions for builds.**

### Triggering Builds

Builds are triggered automatically on push/PR, or manually via:

```bash
gh workflow run msbuild.yml
```

### Monitoring Builds

Only monitor builds if explicitly given permission by the user. To check build status:

```bash
# List recent workflow runs
gh run list --workflow=msbuild.yml --limit 5

# Watch a specific run (get ID from list)
gh run watch <run-id>

# View run details and logs
gh run view <run-id> --log
```

### Build Configurations

The project builds four configurations:
- **Release** - SSE2 optimizations, standard fonts
- **ReleaseAVX2** - AVX2 optimizations for modern CPUs
- **ReleaseFreetype** - SSE2 + Freetype text rendering with custom fonts (larger DLL)
- **ReleaseFreetypeAVX2** - AVX2 + Freetype

All builds target x64 platform and output DLL + PDB artifacts to `output/x64/<Configuration>/`.

## Architecture

### Core Structure

- **Core** (`src/Core/`) - Entry point, initialization, and main loop
- **SDK** (`src/SDK/`) - Game engine interfaces, definitions, and helper utilities
- **Hooks** (`src/Hooks/`) - Function hooks organized by class/method (e.g., `CHLClient_CreateMove.cpp`)
- **Features** (`src/Features/`) - Modular feature implementations
- **Utils** (`src/Utils/`) - Cross-cutting utilities (memory, math, signatures, hashing)

### Hook System

Each hook file in `src/Hooks/` intercepts one Source engine function using the `MAKE_HOOK` macro. Hooks are named by their target: `CClassName_MethodName.cpp`. The hook system:

1. Finds function addresses via signatures (`MAKE_SIGNATURE`)
2. Installs detours with `MAKE_HOOK(Name, Address, ReturnType, ...params)`
3. Calls original via `CALL_ORIGINAL(rcx, ...params)`
4. Uses `DEBUG_RETURN` macro to allow feature debugging without hooks

Key hooks:
- `CHLClient_CreateMove` - Main per-tick logic, runs aimbot/backtrack/misc features
- `IVModelRender_DrawModelExecute` - Visual feature rendering (chams, glow)
- `CClientModeShared_DoPostScreenSpaceEffects` - Post-processing effects
- `IEngineVGui_Paint` - HUD/UI rendering, ImGui integration

### Feature System

Features use the `ADD_FEATURE(Type, Name)` macro to register singletons in the `F` namespace. Each feature:

- Has a class with `Run()` or similar entry points called from hooks
- Stores state as member variables
- Accesses game entities via `H::Entities` helper
- Accesses CVars via `H::ConVars`

Feature categories:
- **Aimbot** - Hitscan/projectile/melee targeting with prediction (`src/Features/Aimbot/`)
- **Visuals** - ESP, chams, glow, materials, notifications, arrows (`src/Features/Visuals/`)
- **Backtrack** - Lag compensation exploitation (`src/Features/Backtrack/`)
- **Simulation** - Projectile/movement prediction for aimbot (`src/Features/Simulation/`)
- **NoSpread** - Weapon spread compensation (`src/Features/NoSpread/`)
- **CritHack** - Critical hit manipulation (`src/Features/CritHack/`)
- **AntiCheatCompatibility** - VAC/CAC evasion techniques (`src/Features/AntiCheatCompatibility/`)

### SDK Layer

`src/SDK/SDK.h` is the central include. Key components:

- **Definitions** - Game classes, interfaces, enums (in `SDK/Definitions/`)
- **Helpers** - ConVars, Entities, Draw, TraceFilters, Particles (in `SDK/Helpers/`)
- **Globals** - Per-frame state in `G` namespace (e.g., `G::CurrentUserCmd`, `G::CanPrimaryAttack`)
- **Vars** - Configuration variables in `Vars` namespace

Important SDK functions:
- `SDK::GetGravity()` - Use this instead of direct `sv_gravity` access
- `SDK::Output()` - Logging with multiple output targets (console, toast, chat, party)
- `H::Entities.GetLocal()`, `H::Entities.GetWeapon()` - Entity accessors
- `H::Entities.GetOrigins()`, `H::Entities.SetAvgVelocity()` - Backtrack data

### Rendering Pipeline

1. **CHLClient_CreateMove** - Per-tick game logic, updates feature state
2. **CClientModeShared_DoPostScreenSpaceEffects** - Chams/glow rendering via `F::Chams.RenderMain()`, `F::Glow.RenderFirst()`
3. **IVModelRender_DrawModelExecute** - Intercepts model rendering for chams/glow effects
4. **CViewRender_DrawViewModels** - Viewmodel effects, finalize with `F::Glow.RenderSecond()`
5. **IEngineVGui_Paint** - ImGui rendering for menu, notifications, and HUD overlays

### Chams System (Recently Reworked)

The chams system uses a two-model rendering technique:
- **Visible pass** - Renders entities normally
- **Occluded pass** - Renders entities behind walls with depth range manipulation

Key features:
- `m_mEntities` tracks which entities are being rendered
- `bTwoModel` parameter controls whether to render both passes
- `$blockoccluded` VMT variable prevents occluded rendering for specific materials
- Supports backtrack and fakeangle chams
- Uses stencil buffer to prevent overlapping renders

### Notifications System (Recently Reworked)

Moved from `src/Features/Visuals/Notifications/` to `src/Features/ImGui/Notifications/` for better integration with ImGui. Now supports icon rendering and uses easing animations.

## Common Patterns

### Adding a New Feature

1. Create files in `src/Features/YourFeature/YourFeature.{h,cpp}`
2. Define class with methods, use `ADD_FEATURE(CYourFeature, YourFeature)`
3. Add includes to relevant hook files (e.g., `CHLClient_CreateMove.cpp`)
4. Call feature from hook: `F::YourFeature.Run(pLocal, pWeapon, pCmd);`
5. Update `Amalgam.vcxproj` and `Amalgam.vcxproj.filters` to include new files

### Adding a New Hook

1. Create `src/Hooks/CClassName_MethodName.cpp`
2. Use `MAKE_SIGNATURE(Name, "client.dll", "pattern", offset)` to find function
3. Use `MAKE_HOOK(Name, S::Name(), ReturnType, params)` to install hook
4. Implement with `DEBUG_RETURN` at start, `CALL_ORIGINAL` to chain
5. Add to `Amalgam.vcxproj`

### Math Optimizations

When optimizing math operations:
- Use `SDK::GetGravity()` instead of direct convar access
- Prefer `powf()` for floating-point exponentiation over manual multiplication
- Use SIMD intrinsics for vector operations where possible
- Cache frequently accessed values in local variables
- Avoid redundant calculations in hot loops (e.g., CreateMove hook runs every tick)

## Upstream Sync

The upstream repository is `rei-2/Amalgam`. To integrate upstream changes:

```bash
# If not already added:
git remote add upstream https://github.com/rei-2/Amalgam.git

# Fetch and merge upstream changes
git fetch upstream
git checkout master
git merge upstream/master
git checkout experimental  # or your current branch
git merge master
# Resolve conflicts, commit
```

Common conflict areas:
- Chams system (entity tracking architecture)
- Velocity calculations (gravity handling)
- Hook file renames (check vcxproj)
- Notifications system (moved to ImGui)

## Branch Strategy

- `master` - Tracks upstream rei-2/Amalgam
- `experimental` - Local optimizations and enhancements (formerly `minimax-optimizations`)

Always branch from `experimental` for new features. Merge upstream changes into `master` first, then into `experimental`.

## Git Workflow

When committing code changes:
```bash
git add <files>
git commit -m "Description of changes"
# Push and create PR
git push origin <branch-name>
gh pr create --title "Title" --body "Description"
```

Do not force push or use destructive git operations without explicit user permission.

## Debugging

The project includes a Debug feature (`src/Features/Debug/`) added in recent upstream changes. Use the `DEBUG_RETURN` macro in hooks to bypass feature logic during debugging:

```cpp
MAKE_HOOK(Name, Address, ReturnType, params)
{
    DEBUG_RETURN(Name, params);  // Returns early if debug mode active
    // ... feature logic
}
```

## Performance Considerations

This codebase runs in a performance-critical context (game hooks executing every frame/tick):

- **CreateMove hook** runs 66 times per second (TF2's tickrate)
- **Rendering hooks** run every frame (~60-300 fps)
- Cache entity lookups, avoid redundant calculations
- Profile hot paths before optimizing
- Document performance-critical sections with comments

See `PERFORMANCE_OPTIMIZATIONS.md` for detailed optimization work.
