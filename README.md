# CrowdBase 0.2.0

An analog-style crowd instrument for continuous sports and audience ambience.
Windows x64 VST3 instrument and standalone application. Generated raster faceplates,
three finishes, physical rotary controls, cyan VFD values and a live peak meter.

## Use in Studio Pro / Studio One

Extract the Windows package and run `Install-CrowdBase.cmd`. Restart or rescan your
instrument list and add **CrowdBase** to an instrument track. No MIDI clip or audio
input is needed: FOLLOW HOST produces the crowd while transport is playing.
The standalone uses the play and stop icon buttons; AUDIO selects an output device.

Start with **Mellow stands**. Automate **Energy** for the emotional shape of a scene.
For a sudden huge play, automate **Big Play Trigger** from 0 to 1, then back to 0,
or send MIDI note 36 (C1 in some hosts, C2 in others). A held trigger fires once.
The red BIG PLAY button sends the same host-notified pulse; your DAW decides whether
to record that automation according to its write/touch mode.

An example envelope: leave Energy at 18%, lift it to 40% during anticipation,
trigger at the play, and leave Energy at 18% while the reaction settles.
**Time to Peak** sets the rise from trigger to maximum reaction, from **0.05 to 8
seconds** (default **0.15 s**). Use a short rise for an explosive play, or a longer
rise for a crowd that gradually realizes what happened. The setting is captured
at each trigger, so moving it during a reaction sets the next reaction's rise.
The cyan display reads seconds; double-click the slider to restore the fast default.
Settle controls the return time after the peak; it does not force the underlying
Energy automation back to a preset value. Re-triggering rises from the current
reaction level without abruptly dropping the crowd. Both controls save and automate
with the DAW; sessions from 0.1.0 keep a fast rise and their existing automation.

## Automatable controls

| Control | What it changes |
|---|---|
| Energy | Mellow-to-excited blend, applause/whistle activity and reaction intensity |
| Population | Ensemble density, with level compensation |
| Crowd Bed | Underlying diffuse audience texture |
| Cheers | Excited ensemble layer; Energy determines how much it emerges |
| Applause | Additional independently synthesized palm claps, sparse in mellow passages |
| Whistles | Rate of sustained procedural human-style whistles, with small pitch drift and breath texture |
| Distance | Softer high frequencies and a more diffuse perspective |
| Space | Added stadium reflections and decay |
| Width | Mono at 0%; full stereo at 100% |
| Tone | Broad darker-to-brighter balance |
| Variation | Slow, irregular emotional drift around Energy |
| Time to Peak | Big-play rise, 0.05–8 seconds; captured on each trigger |
| Settle | Big-play decay, approximately 1–25 seconds |
| Output | -36 to +6 dB, with smooth saturation protection |
| Big Play Trigger | Rising-edge one-shot reaction; reset to 0 to arm again |
| Free Run | Generate even while DAW transport is stopped; disabled by default |

The ensemble sources contain diffuse background applause. Applause controls the
additional handclap layer, rather than separating every clap from that texture.
Space adds reflections to an already ambient source; it is not a dry close-mic model.

## How the synthesis works

This is granular resynthesis of original **synthesized crowd ensembles**, plus
procedural claps and whistles. It is not a recorded stadium loop and uses no film
audio, announcers, or isolated stock TTS syllables. The 124-second stereo atlas was
prepared from the earlier reference-matched and huge-play procedural crowd renders.
Several independent, slightly varied overlapping passages keep evolving; no entire
minute repeats and automation never restarts an underlying file loop.

Spectator whistles are generated independently from the ensemble atlas. Longer
steady notes, faint breath turbulence, narrow pitch drift and occasional breath
breaks replace the earlier short, sweeping bird-like tones.

The first prototype's isolated voice grains were rejected after listening feedback.
The current engine uses ensemble textures to avoid exposed robotic "maa" syllables.
Naturalness remains subject to listening feedback; numerical checks do not certify it.

All continuous controls are smoothed over about 75 ms. The plugin has no lookahead
or reported latency. Transport fades over 45 ms, then freezes the population until
play resumes. Host loop wraps and timeline seeks do not reset the sound. It follows
play/stop, rather than reconstructing identical audio for every absolute timeline
position. Render/bounce a take when you need sample-exact edits to a fixed result.

Parameters, finish and the instance's random seed are saved in the DAW project.
The seed makes newly prepared performances reproducible from the same initial state;
the running audio history is not serialized. Separate new instances get separate seeds.
The standalone saves its controls and finish on normal exit; it starts stopped.

## Build and checks

JUCE is vendored under `third_party/JUCE`; C++20 and CMake 3.22+ are required.
On Windows with Visual Studio 2022 C++ tools installed, run `./Build-source.ps1`.
Alternatively use `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`,
then `cmake --build build --parallel` from a configured compiler environment.

`scripts/Test-CrowdBase.ps1` runs DSP checks and a real VST3 host test: moving
playhead, automation, transport gating, state restore, editor creation/closure,
and repeated removal/recreation. Tests render a 60-second mellow/reaction/mellow WAV.
Windows has been exercised locally. No macOS binary is included or claimed tested.

## GitHub releases

Repository: https://github.com/lakam99/CrowdBase

Each push and pull request runs the Windows build and checks. A `vX.Y.Z` tag matching
the CMake version additionally creates a release after the build, audio/host checks
and secret scans pass. Release assets stay in draft until their uploaded sizes and
GitHub SHA-256 digests match the checked files. SHA256SUMS.txt allows independent
download verification. The source ZIP contains the full dependency and build workflow.

Download a release, verify its SHA-256, extract the Windows ZIP and run its installer.
Releases are public; there is no in-app update checker in this initial version.

No model, file loading, locks, thread joins or memory growth are used in the crowd
render callback. Voice storage and the feedback network are allocated in prepare.
Raster assets are retained as software images to avoid GPU resource creation during
VST DLL unload on Windows. GUI activity stays on the message thread.

The skin artwork and icon were produced using built-in imagegen; prompts and asset
provenance are in `resources/generated/PROMPTS.md` and `resources/ATLAS-PROVENANCE.json`.

## Source and licensing

Original CrowdBase code is provided under AGPL-3.0-or-later. JUCE 9 is used under
its AGPLv3 option; see `LICENSE.txt`, `JUCE-LICENSE.md` and the vendored notices.
Generated artwork and original synthesized atlas accompany this project for its use.
The package includes corresponding source and the complete JUCE dependency.
