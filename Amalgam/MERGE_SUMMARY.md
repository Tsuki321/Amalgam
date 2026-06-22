# Upstream Merge Summary

## Branch Status
- **Current branch**: `experimental` (renamed from `minimax-optimizations`)
- **Upstream**: `rei-2/Amalgam` (master branch)
- **Status**: Successfully merged 9 upstream commits

## Integrated Changes

### Major Features
1. **Notifications System Rework**
   - Moved from `src/Features/Visuals/Notifications/` to `src/Features/ImGui/Notifications/`
   - Added notification icon support
   - Integrated with ImGui rendering pipeline

2. **Chams System Overhaul**
   - New two-model rendering architecture
   - Backtrack chams support
   - Fakeangle chams support
   - Added `$blockoccluded` VMT variable for custom chams
   - Improved entity ordering and rendering logic

3. **Bug Fixes**
   - Fixed dormancy bug affecting dead-to-spawned players
   - Fixed physics weapon targeting (prioritizes non-water points and higher positions)
   - Fixed KeyValues allocation size issue

4. **New Features**
   - Added Debug module (`src/Features/Debug/`)
   - Added Easings helper (`src/Features/ImGui/Easings/`)
   - Display weapon and viewmodel flip in player conditions for non-local players
   - Better viewmodel effect support
   - Movement simulation enhancements (reverse average yaw upon landing)

### Build System Changes
- Replaced `CBaseAnimating_UpdateClientSideAnimation.cpp` with `CTFPlayer_UpdateClientSideAnimation.cpp`

## Conflict Resolutions

### 1. Amalgam.vcxproj
- **Issue**: Hook file renamed
- **Resolution**: Updated to use `CTFPlayer_UpdateClientSideAnimation.cpp`

### 2. Chams.cpp
- **Issue**: Different entity tracking architecture
- **Resolution**: Adopted master's new architecture with `m_mEntities` tracking and two-model support

### 3. CBaseEntity_SetAbsVelocity.cpp
- **Issue**: Gravity calculation differences
- **Resolution**: Replaced direct `sv_gravity` convar access with `SDK::GetGravity()` and used `powf()` for consistency

### 4. CClientModeShared_DoPostScreenSpaceEffects.cpp
- **Issue**: Entity tracking reset call
- **Resolution**: Removed obsolete `m_bEntities.reset()` call (handled differently in new architecture)

### 5. IVModelRender_DrawModelExecute.cpp
- **Issue**: Viewmodel detection method
- **Resolution**: Updated to use `IsWearableVM()` instead of class ID check

## Local Changes Preserved
- All performance optimizations from experimental branch
- Vector math optimizations
- Aimbot improvements
- Backtrack enhancements
- Optimization documentation files

## Next Steps
1. Test the build to ensure compilation succeeds
2. Run the application to verify runtime behavior
3. Test interactions between upstream changes and local optimizations
4. Consider pushing to remote if tests pass

## Commands Used
```bash
git remote add upstream https://github.com/rei-2/Amalgam.git
git fetch upstream
git checkout master
git merge upstream/master
git checkout minimax-optimizations
git branch -m minimax-optimizations experimental
git merge master
# Resolved conflicts manually
git commit
```
