# AGENTS.md — StretchKit

Instructions for coding agents (Codex etc.) working in this repository.
Background, decisions and history are in `docs/HANDOFF.md` — read it before changing behaviour.

## What this is
StretchKit: a free, open-source (GPLv3) drum **instrument** plugin, made by **@sxdrv.mp3**.
- XO-style sample map: scans drum folders, classifies one-shots, lays them out as one cloud.
- Drum time-stretch per track (LENGTH % of the original sample), Hybrid mode keeps the attack.
- 8-track step sequencer synced to the host, MIDI drag-out, 5 selectable skins.
- Formats: VST3 + AU + Standalone. macOS universal (arm64 + x86_64), Windows x64.
- **No VST2** (Steinberg no longer licenses it). Do not add it.

Stack: C++17, JUCE 8.0.10 (AGPLv3 terms), Rubber Band Library v4.0.0 (GPLv2+, single-file build),
both fetched by CMake FetchContent. Python (numpy/scipy/pillow/reportlab) only for tools.

## Layout
```
Source/
  Common.h                  categories, colours, map world coords (kWorldW x 1), category cloud centres
  dsp/DrumStretcher         offline render: resample + Rubber Band R3; Hybrid = attack untouched, tail stretched
  library/FeatureExtractor  10 features per one-shot + rule-based classification (name first, then sound)
  library/SampleLibrary     background scan (multi-threaded), binary cache, resume, layout, nearest(), favourites
  PluginProcessor           sequencer, voices, render thread, preview (crossfaded), state, auditionTrack()
  PluginEditor              layout (resizable), header, search, similarity bar wiring, transport, tracks
  ui/Theme                  5 skins (ThemeId), SkLookAndFeel, fonts, chrome icons, drawing helpers
  ui/MapView                cached pixel-rendered dot cloud, scrub/drag-out/pan/zoom, filters, hover
  ui/SimilarityBar          seed + 24 nearest, step/play-through, history, heart, load
  ui/TrackRow               track strip + StepGrid
Resources/fonts, Resources/icons   embedded via juce_add_binary_data (BinaryData.h)
Tests/
  Tests.cpp      DSP + classification (no GUI)            -DSTRETCHKIT_BUILD_TESTS=ON
  HostTest.cpp   processor/editor end-to-end              -DSTRETCHKIT_BUILD_HOST_TEST=ON
  Preview.cpp    renders docs/skin_*.png with a demo kit  (same option)
  Stress.cpp     large library: speed, interrupted scan, resume, reload   (same option)
tools/
  chrome_icons.py   SDF ray-marcher that renders Resources/icons/*.png (liquid chrome)
  icon_sheet.py     docs/chrome_icons.png
  manual/           PDF manuals (content.py = EN/KO text, build.py, make_images.py)
.github/workflows/build.yml   CI: mac universal AU/VST3/Standalone + win VST3/Standalone, runs DSP tests
```

## Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release                    # add "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" on mac
cmake --build build --config Release --parallel
# tests
cmake -B build -DSTRETCHKIT_BUILD_TESTS=ON -DSTRETCHKIT_BUILD_HOST_TEST=ON
cmake --build build --target StretchKitTests StretchKitHostTest
./build/StretchKitTests_artefacts/Release/StretchKitTests
HOME=/tmp/sk xvfb-run -a ./build/StretchKitHostTest_artefacts/Release/StretchKitHostTest /tmp/sk   # Linux needs a display
```
Linux build deps: libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev
libxcomposite-dev libfreetype-dev libfontconfig1-dev libgl-dev, plus xvfb for GUI tests.
Offline? pass `-DFETCHCONTENT_SOURCE_DIR_JUCE=... -DFETCHCONTENT_SOURCE_DIR_RUBBERBAND=...`.

Regenerate docs: Preview tool -> `docs/skin_*.png`; `python3 tools/manual/make_images.py tools/manual .`
then `python3 tools/manual/build.py tools/manual . docs/manual`.

## Rules that matter
- **Audio thread never allocates or frees.** Rendered buffers are `RenderedSample` objects kept alive by the
  processor's pool; the audio thread only swaps pointers (`pending[]`, `pendingPreview`) and marks them `taken`;
  `timerCallback()` frees unused ones on the message thread. Keep it that way.
- All stretching happens on the render thread (`RenderThread`); `settingsFor()` / `checkRenderNeeded()` decide re-renders.
- **macOS/clang gotcha:** with juce_dsp included, `juce::jmin<size_t>`, `juce::jmax<juce::int64>` etc. pick up
  SIMDRegister overloads and fail to compile on Apple clang (not on GCC). Use `std::min/std::max` for non-int/float types.
- Warnings are treated seriously (JUCE recommended warning flags): no shadowing, no float `==`.
- Library cache: `~/Library/Application Support/StretchKit/library.bin` (mac), `%APPDATA%/StretchKit/` (win),
  `~/.config/StretchKit/` (linux). Format is versioned (`kCacheMagic`, `kCacheVersion`); bump the version if you change it.
  Settings (default skin) in `StretchKit.settings` next to it.
- Plugin identity (don't change after release — DAWs treat a change as a new plugin): COMPANY_NAME `sxdrv.mp3`,
  BUNDLE_ID `com.stretchkit.stretchkit`, manufacturer code `Stkt`, plugin code `Stk1`.
- Licences: keep GPLv3 for our code; JUCE under AGPLv3 terms; Rubber Band GPL (closed-source would need a commercial
  licence); fonts are OFL (licence texts in `Resources/fonts/licenses`). Never copy code/assets from XO, Serato or other
  commercial products; implement ideas independently.
- Commit author for agent commits: whatever the repo owner prefers; CI runs on every push to `main`.

## Known limits / ideas (see HANDOFF.md)
- Classification is rule-based (~75% right by sound alone on synthetic tests; names usually decide). A learned model
  (trained on the user's name-labelled folders) would be the biggest improvement.
- Map layout is per-category PCA -> rank-filled discs (no t-SNE). XO uses t-SNE approximated by a neural net.
- Releases cannot be created by the agent session; the owner publishes them on GitHub (CI attaches the zips).
- Not yet verified on a real 120k-sample library or inside every DAW.
