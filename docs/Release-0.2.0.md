# CrowdBase 0.2.0

- Reworked spectator whistles into sustained human-style whistles with stable pitch,
  subtle breath and restrained variation, replacing the short sweeping chirps.
- Added a hardware-style **Time to Peak** slider and VFD readout: **0.05–8 seconds**,
  default **0.15 seconds**. Automate it to choose an explosive or gradual big-play rise.
- The rise is captured at each trigger; Settle controls the return after the peak.
  Re-triggering builds from the current level without an abrupt dip.
- Preserved existing parameter IDs and positions; older sessions load the fast rise.
- Kept all three analog raster finishes, transport following and continuous synthesis.

Download **CrowdBase-0.2.0-Windows-x64.zip**, extract and run **Install-CrowdBase.cmd**.
Close CrowdBase and any DAW using it before installing. The package includes the
standalone, VST3, corresponding source, audio example and verification reports.

Whistle timbre is still procedural; the included preview is available for listening.
No macOS binary is included in this release.
