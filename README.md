<p align="center">
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64Release.zip">
    <img src=".github/assets/download.png" alt="Download" width="auto" height="auto">
  </a>
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64ReleasePDB.zip">
    <img src=".github/assets/pdb.png" alt="PDB" width="auto" height="auto">
  </a>
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64ReleaseAVX2.zip">
    <img src=".github/assets/download_avx2.png" alt="Download AVX2" width="auto" height="auto">
  </a>
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64ReleaseAVX2PDB.zip">
    <img src=".github/assets/pdb_avx2.png" alt="PDB AVX2" width="auto" height="auto">
  </a>
  <br>
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64ReleaseFreetype.zip">
    <img src=".github/assets/freetype.png" alt="Download Freetype" width="auto" height="auto">
  </a>
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64ReleaseFreetypePDB.zip">
    <img src=".github/assets/pdb.png" alt="PDB Freetype" width="auto" height="auto">
  </a>
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64ReleaseFreetypeAVX2.zip">
    <img src=".github/assets/freetype_avx2.png" alt="Download Freetype AVX2" width="auto" height="auto">
  </a>
  <a href="https://nightly.link/rei-2/Amalgam/workflows/msbuild/master/Amalgamx64ReleaseFreetypeAVX2PDB.zip">
    <img src=".github/assets/pdb_avx2.png" alt="PDB Freetype AVX2" width="auto" height="auto">
  </a>
</p>

###### AVX2 may be faster than SSE2 though not all CPUs support it (Steam > Help > System Information > Processor Information > AVX2). Freetype uses freetype as the text rasterizer and includes some custom fonts, which results in better looking text but larger DLL sizes. PDBs are for developer use

# Amalgam

[![GitHub Repo stars](https://img.shields.io/github/stars/rei-2/Amalgam)](/../../stargazers)
[![Discord](https://img.shields.io/discord/1227898008373297223?logo=Discord&label=discord)](https://discord.gg/RbP9DfkUhe)
[![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/rei-2/Amalgam/msbuild.yml?branch=master)](/../../actions)
[![GitHub commit activity (branch)](https://img.shields.io/github/commit-activity/m/rei-2/Amalgam)](/../../commits/)

[VAC bypass](https://github.com/danielkrupinski/VAC-Bypass-Loader) and [Xenos](https://github.com/DarthTon/Xenos/releases) recommended

## AutoShoot Branch Changes

### New Features
- Added a separate Auto Shoot functionality in the Aimbot section
- Auto shoot fires when an enemy player is detected under your crosshair
- Two filter options:
  - Head: Only fires when an enemy head is under the crosshair
  - Torso: Fires when an enemy torso is under the crosshair
- Respects all existing aimbot ignore settings (cloaked players, friends, etc.)
- Compatible with existing headshot waiting logic for Sniper and Ambassador
- Works independently from the main aimbot's aiming systems