# Text of the StretchKit manual in English and Korean.
# Markup: Paragraph XML (b, i, font). Lists are lists of strings. Tables are lists of rows.

RELEASE = "https://github.com/edwina9195/stretchkit/releases/latest"
REPO = "https://github.com/edwina9195/stretchkit"
INSTA = "https://www.instagram.com/sxdrv.mp3/"

EN = {
    "lang": "en",
    "cover_tag": "Drum sample map  ·  natural drum time-stretch  ·  step sequencer",
    "cover_doc": "User Manual &amp; Installation Guide",
    "cover_ver": "Version 0.1.0  ·  macOS &amp; Windows  ·  AU / VST3 / Standalone",
    "made_by": "Made by",
    "footer": "StretchKit v0.1.0  ·  Made by @sxdrv.mp3",
    "contents": "Contents",
    "toc": ["About StretchKit", "Download", "Install on macOS", "Install on Windows", "Find it in your DAW",
            "Quick start", "The screen at a glance", "Sample map", "Similarity bar", "Drum time-stretch",
            "Step sequencer", "Skins", "Troubleshooting", "Licence &amp; credits"],

    "about_title": "About StretchKit",
    "about": "StretchKit is a free drum instrument plugin. It scans your drum sample folders, sorts every one-shot "
             "by how it sounds and lays them out on a map, so similar sounds sit next to each other. Each of its "
             "eight tracks can stretch its sample to any length from 25% to 400% of the original &mdash; keeping "
             "the attack untouched so drums stay punchy &mdash; and a step sequencer plays it all in time with your DAW.",
    "features": [
        "<b>Sample map</b> &mdash; automatic sorting into Kick, Snare, Clap, Hat, Cymbal, Tom, Perc and Other, with similar sounds grouped together. Click and drag to hear every sound you pass.",
        "<b>Similarity bar</b> &mdash; the 24 sounds closest to the one you picked, with history and favourites.",
        "<b>Drum time-stretch</b> &mdash; Hybrid mode keeps the transient sample-for-sample and stretches only the tail (Rubber Band R3).",
        "<b>Step sequencer</b> &mdash; 8 tracks, 8/16/32 steps, accents, swing, choke, synced to your DAW.",
        "<b>Drag MIDI</b> &mdash; drag your pattern straight into the DAW as a MIDI clip.",
        "<b>5 skins</b> &mdash; PS2 Night, Y2K Chrome, Digicore, Mix and Liquid Chrome.",
    ],
    "req_title": "Requirements",
    "req": [["", "macOS", "Windows"],
            ["System", "macOS 10.13 or later, Apple Silicon or Intel", "Windows 10 / 11, 64-bit"],
            ["Formats", "AU, VST3, Standalone", "VST3, Standalone"],
            ["Host", "Logic Pro, GarageBand, Ableton Live, FL Studio, Cubase, Bitwig, Reaper, Studio One...", "Any 64-bit VST3 host"]],
    "no_vst2": "<b>Why no VST2?</b> Steinberg stopped issuing VST2 licences in 2018, so new plugins can't be released "
               "as VST2. Every current DAW loads VST3 (and Logic / GarageBand use AU).",

    "dl_title": "Download",
    "dl": "All files are on the releases page. Download only what you need:",
    "dl_link": RELEASE,
    "dl_files": [["File", "What it is"],
                 ["StretchKit-mac-AU.zip", "macOS Audio Unit &mdash; Logic Pro, GarageBand"],
                 ["StretchKit-mac-VST3.zip", "macOS VST3 &mdash; Ableton, FL Studio, Cubase, Bitwig, Reaper..."],
                 ["StretchKit-mac-Standalone.zip", "macOS app that runs without a DAW"],
                 ["StretchKit-win-VST3.zip", "Windows VST3 plugin"],
                 ["StretchKit-win-Standalone.zip", "Windows app that runs without a DAW"]],

    "mac_title": "Install on macOS",
    "mac_steps": [
        "Double-click the zip file(s) you downloaded to unzip them.",
        "In Finder choose <b>Go › Go to Folder...</b> and paste the folder path for your format:",
    ],
    "mac_paths": [["Format", "Put this", "Into this folder"],
                  ["AU", "StretchKit.component", "~/Library/Audio/Plug-Ins/Components/"],
                  ["VST3", "StretchKit.vst3", "~/Library/Audio/Plug-Ins/VST3/"],
                  ["Standalone", "StretchKit.app", "/Applications/"]],
    "mac_unblock": "StretchKit is free and not signed through Apple's paid developer programme, so macOS blocks files "
                   "downloaded from the internet. Open <b>Terminal</b> (Applications › Utilities) and run the lines "
                   "for the formats you installed &mdash; this only removes the download flag from these files:",
    "mac_cmds": "xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/StretchKit.component\n"
                "xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/StretchKit.vst3\n"
                "xattr -dr com.apple.quarantine /Applications/StretchKit.app",
    "mac_after": "Restart your DAW. If Logic Pro still doesn't list it, run the line below and restart Logic:",
    "mac_cmd2": "killall -9 AudioComponentRegistrar",

    "win_title": "Install on Windows",
    "win_steps": [
        "Right-click the downloaded zip and choose <b>Extract All...</b>",
        "Copy the <b>StretchKit.vst3</b> folder into <font name='Courier'>C:\\Program Files\\Common Files\\VST3\\</font> "
        "(click <b>Continue</b> if Windows asks for administrator permission).",
        "Standalone: put <b>StretchKit.exe</b> anywhere you like. If SmartScreen says &ldquo;Windows protected your PC&rdquo;, "
        "click <b>More info › Run anyway</b> (the app is unsigned, not unsafe).",
        "Restart your DAW and let it scan for new plugins.",
    ],
    "win_note": "No extra runtimes are needed &mdash; the plugin includes everything it uses.",

    "daw_title": "Find it in your DAW",
    "daw_intro": "StretchKit is an <b>instrument</b>: put it on a MIDI / instrument track. It is listed under the maker "
                 "<b>sxdrv.mp3</b>. Menu names can differ slightly between DAW versions.",
    "daw": [["DAW", "What to do"],
            ["Ableton Live", "Settings › Plug-Ins: turn on <i>Use VST3 Plug-in System Folders</i> (Mac: also <i>Use Audio Units</i>), then <b>Rescan</b>. Find it under Plug-Ins › sxdrv.mp3."],
            ["FL Studio", "Options › Manage plugins › <b>Find installed plugins</b>, then add it from the Channel Rack (+)."],
            ["Logic Pro / GarageBand", "Uses the AU version. Instrument slot › AU Instruments › sxdrv.mp3 › StretchKit. If missing: Settings › Plug-in Manager › Reset &amp; Rescan Selection."],
            ["Cubase / Nuendo", "Scans on start-up. Check Studio › VST Plug-in Manager if it doesn't show."],
            ["Bitwig Studio", "Settings › Locations: make sure VST3 is enabled, then rescan."],
            ["Reaper", "Options › Preferences › Plug-ins › VST › <b>Re-scan</b>."],
            ["Studio One", "Scans on start-up. Options › Locations › VST Plug-ins to check the folder list."]],

    "qs_title": "Quick start",
    "qs": [
        "Load StretchKit on an instrument track.",
        "Click <b>Add Folder</b> and choose the folder with your drum samples. The map fills in while it scans.",
        "On first use, StretchKit loads a random starter kit from your library onto the 8 tracks.",
        "Press <b>Play</b> in your DAW (or StretchKit's own Play button) &mdash; the starter pattern plays.",
        "Click and drag across the map to hear sounds; keep dragging out of the map onto a track to load the last one.",
        "Drag <b>LENGTH %</b> on a track and let go &mdash; the track plays its new, stretched sound. Click a track name to hear it again.",
    ],

    "ov_title": "The screen at a glance",
    "ov": ["Add Folder / Folders / Rescan &mdash; manage the folders that are scanned",
           "Skin &mdash; switch the look (remembered for next time)",
           "Category filters &mdash; show or hide each kind of sound on the map",
           "Search / Favourites &mdash; filter the map by name or by hearts",
           "Sample map &mdash; every one-shot as a dot; scan progress and count bottom-left",
           "Similarity bar &mdash; the picked sound and its 24 closest sounds",
           "History, heart, load to the selected track",
           "Play / Stop &mdash; runs the sequencer when the DAW is stopped",
           "Stretch mode &mdash; Hybrid, Smooth or Tape",
           "Drag MIDI &mdash; drag the pattern into your DAW",
           "Track &mdash; drop samples here; click to select and hear the track's own sound",
           "Rnd (dice) &mdash; swap for a similar sound; M &mdash; mute",
           "VOL / PAN",
           "PITCH / LENGTH % &mdash; tuning and time-stretch",
           "Steps &mdash; the pattern for this track"],

    "map_title": "Sample map",
    "map_text": [
        "<b>Adding folders.</b> Click <b>Add Folder</b> and pick a folder; sub-folders are included. "
        "Use <b>Folders</b> to remove one and <b>Rescan</b> after adding new files. WAV, AIFF, FLAC and OGG files up to "
        "10 seconds long are included (longer files are treated as loops and skipped).",
        "<b>You only scan once.</b> Folders and results are saved on your computer and shared by every StretchKit you open: "
        "the next time, the map appears straight away with nothing to load. A large library is analysed on several CPU cores "
        "and its progress is saved as it goes &mdash; if you close the plugin or the DAW in the middle, the scan carries on by itself "
        "the next time StretchKit opens, skipping everything already done. Rescan only looks at new or changed files.",
        "<b>How sounds are sorted.</b> File and folder names come first (&ldquo;kick&rdquo;, &ldquo;BD_01&rdquo;, &ldquo;Snares/...&rdquo;). "
        "When a name gives no hint, StretchKit listens to the sound itself: low-end energy, brightness, noisiness, decay "
        "time and the multiple bursts of a clap. Inside each group, similar sounds are placed close together.",
    ],
    "map_controls": [["Do this", "To"],
                     ["Click a dot", "Listen, and show its similar sounds in the similarity bar"],
                     ["Click and drag across the map", "Hear every sound you pass over"],
                     ["Keep dragging out of the map onto a track", "Load the last sound you heard on that track"],
                     ["Double-click a dot", "Load it on the selected track"],
                     ["Mouse wheel / trackpad pinch", "Zoom in and out"],
                     ["Two-finger scroll, Cmd-drag or right-drag", "Move around (double-click empty space to reset)"],
                     ["Search box / Favourites", "Show only names containing your words / only hearted sounds"],
                     ["Category buttons at the top", "Show or hide that category"]],
    "map_drop": "You can also drag audio files straight from Finder / Explorer onto a track.",

    "sim_title": "Similarity bar",
    "sim_text": "The strip under the map shows the sound you picked (big ring) and the 24 sounds that are closest to it, "
                "nearest first. The same dots light up on the map.",
    "sim_controls": [["Control", "What it does"],
                     ["|&lt;  &lt;  &gt;", "Back to the picked sound / previous / play the similar sounds one after another (press again to stop)"],
                     ["A dot", "Click to hear it; drag it onto a track to load it"],
                     ["History &#9650; &#9660;", "Go back and forward through the sounds you picked before"],
                     ["Heart", "Mark the sound as a favourite (saved). Use <b>Favourites</b> above the map to show only these"],
                     ["Load", "Put the sound on the selected track and play it"]],

    "st_title": "Drum time-stretch",
    "st_text": "<b>LENGTH %</b> sets each track's length relative to the <b>original sample</b>: 50% is half as long, "
               "200% twice as long (range 25&ndash;400%, double-click for 100%). <b>PITCH</b> tunes the sample in semitones "
               "(&plusmn;24); LENGTH % still refers to the original length, so you can tune and stretch independently.",
    "st_modes": [["Mode", "What it does", "Best for"],
                 ["Hybrid (default)", "Keeps the attack exactly as recorded and stretches only the tail.", "Kicks, snares, claps &mdash; anything punchy"],
                 ["Smooth", "Stretches the whole sample with Rubber Band R3.", "Long, tonal or textured sounds"],
                 ["Tape", "Changes speed: longer = lower, shorter = higher, like a slowed record.", "Lo-fi and pitched-down effects"]],
    "st_note": "Stretching is calculated in the background when a sample or setting changes, so playback itself uses almost "
               "no CPU and adds no latency. When you let go of PITCH or LENGTH %, the track plays its new sound once; click "
               "the track name to hear it again. Note: with <b>Choke</b> on, a following hit on the same track cuts a long tail &mdash; "
               "turn Choke off to hear long stretches ring out in a pattern.",

    "seq_title": "Step sequencer",
    "seq_steps": [["Do this", "To"],
                  ["Click a step", "Turn it on / off"],
                  ["Right-click or Shift-click", "Make it an accent (louder, marked with a highlight)"],
                  ["Click and drag across steps", "Paint several at once"]],
    "seq_transport": [["Control", "What it does"],
                      ["Play / Stop", "Runs the sequencer when the DAW is stopped. When the DAW plays, StretchKit follows it (button shows <i>Host</i>)."],
                      ["BPM", "Tempo for the internal clock; the DAW's tempo is used whenever it provides one."],
                      ["Steps / Rate", "Pattern length (8, 16, 32) and step size (1/8, 1/16, 1/32, 1/16 triplets)."],
                      ["Swing", "Delays every second step for a shuffled groove."],
                      ["Choke", "A new hit on a track fades out the previous one. Turn off to let stretched tails ring over each other."],
                      ["Master", "Overall output level."],
                      ["Drag MIDI", "Drag onto a MIDI track in your DAW to get the pattern as a clip."]],
    "seq_midi": "<b>Playing with MIDI.</b> Notes 36&ndash;43 play tracks 1&ndash;8 (C1&ndash;G1 in most DAWs; some, such as FL Studio, "
                "call the same notes C3&ndash;G3). Velocity sets the loudness.",

    "sk_title": "Skins",
    "sk_text": "Pick a look from the <b>SKIN</b> menu at the top. Your last choice is used for new instances, and each project "
               "remembers its own skin.",
    "skins": [("skin_A_ps2_night.png", "PS2 Night", "Deep navy, fog, glass towers and menu lights."),
              ("skin_B_y2k_chrome.png", "Y2K Chrome", "Pearl silver, chrome capsules, jelly steps."),
              ("skin_C_digicore.png", "Digicore", "Black and neon, pixel fonts, glitch, scanlines."),
              ("skin_D_mix.png", "Mix", "PS2 night with Y2K chrome and digicore sparkle."),
              ("skin_E_liquid_chrome.png", "Liquid Chrome", "Blue-violet glass with 3D liquid-chrome icons.")],

    "ts_title": "Troubleshooting",
    "ts": [("macOS says the plugin is damaged or can't be opened.", "Run the Terminal lines in <i>Install on macOS</i>, then restart the DAW."),
           ("StretchKit doesn't appear in my DAW.", "Check the folder (section 3 or 4), make sure VST3 / AU scanning is enabled and rescan. StretchKit is an instrument, so look in the instrument list."),
           ("I hear nothing.", "Press Play in the DAW or StretchKit, check that the tracks have samples and are not muted (M), and that the track's steps are on."),
           ("The map is empty.", "Add a folder with Add Folder. Only one-shots up to 10 seconds in WAV / AIFF / FLAC / OGG are included. Scan progress is shown bottom-left."),
           ("I closed the plugin while it was scanning.", "Nothing is lost. The scan continues by itself the next time StretchKit opens, starting where it stopped."),
           ("LENGTH or PITCH doesn't seem to change anything.", "Click the track name: it plays the track's own sound. In a pattern, Choke cuts long tails when the next hit comes &mdash; turn it off to hear them."),
           ("A track says &ldquo;(missing)&rdquo;.", "The file was moved or renamed. Load it again from the map or press Rescan."),
           ("A sound is in the wrong category.", "Names win: renaming the file or its folder (e.g. adding &ldquo;kick&rdquo;) and pressing Rescan fixes it."),
           ("Windows warns about the standalone app.", "Click More info › Run anyway. The app is unsigned, which is normal for free plugins.")],

    "lic_title": "Licence &amp; credits",
    "lic": [
        "StretchKit is free and open source under the <b>GNU General Public License v3</b>. Source code: " + REPO,
        "Built with <b>JUCE 8</b> (used under AGPLv3) and the <b>Rubber Band Library</b> by Particular Programs Ltd. (GPLv2 or later). "
        "Fonts: Rajdhani, Michroma, VT323, Silkscreen, Press Start 2P and Inter (SIL Open Font License).",
        "VST is a trademark of Steinberg Media Technologies GmbH. Audio Units and macOS are trademarks of Apple Inc.",
    ],
    "credit_line": "Made by",
}

KO = {
    "lang": "ko",
    "cover_tag": "드럼 샘플 지도  ·  자연스러운 드럼 타임스트레치  ·  스텝 시퀀서",
    "cover_doc": "사용 설명서 &amp; 설치 가이드",
    "cover_ver": "버전 0.1.0  ·  macOS &amp; Windows  ·  AU / VST3 / Standalone",
    "made_by": "Made by",
    "footer": "StretchKit v0.1.0  ·  Made by @sxdrv.mp3",
    "contents": "목차",
    "toc": ["StretchKit 소개", "다운로드", "맥 설치", "윈도우 설치", "DAW에서 찾기", "빠른 시작", "화면 한눈에 보기",
            "샘플 지도", "유사도 바", "드럼 타임스트레치", "스텝 시퀀서", "스킨", "문제 해결", "라이선스 &amp; 크레딧"],

    "about_title": "StretchKit 소개",
    "about": "StretchKit은 무료 드럼 악기 플러그인이에요. 드럼 샘플 폴더를 스캔해서 원샷을 소리 특성별로 분류하고, "
             "비슷한 소리끼리 가깝게 지도에 펼쳐 보여줘요. 8개 트랙은 각각 샘플 길이를 원본의 25%~400%로 늘이거나 줄일 수 "
             "있는데, 어택은 그대로 두기 때문에 드럼의 펀치감이 살아 있어요. 스텝 시퀀서는 DAW 템포에 맞춰 함께 재생돼요.",
    "features": [
        "<b>샘플 지도</b> &mdash; Kick, Snare, Clap, Hat, Cymbal, Tom, Perc, Other로 자동 분류하고 비슷한 소리끼리 모아줘요. 클릭한 채 드래그하면 지나가는 소리가 다 재생돼요.",
        "<b>유사도 바</b> &mdash; 고른 소리와 가장 비슷한 24개를 보여주고, 히스토리와 즐겨찾기도 있어요.",
        "<b>드럼 타임스트레치</b> &mdash; Hybrid 모드는 어택을 원본 그대로 두고 꼬리만 늘여요(Rubber Band R3).",
        "<b>스텝 시퀀서</b> &mdash; 8트랙, 8/16/32스텝, 액센트, 스윙, 초크, DAW 동기화.",
        "<b>Drag MIDI</b> &mdash; 만든 패턴을 DAW로 바로 끌어다 MIDI 클립으로 쓸 수 있어요.",
        "<b>스킨 5종</b> &mdash; PS2 Night, Y2K Chrome, Digicore, Mix, Liquid Chrome.",
    ],
    "req_title": "지원 환경",
    "req": [["", "macOS", "Windows"],
            ["시스템", "macOS 10.13 이상, Apple Silicon·Intel 모두", "Windows 10 / 11, 64비트"],
            ["포맷", "AU, VST3, Standalone", "VST3, Standalone"],
            ["호스트", "Logic Pro, GarageBand, Ableton Live, FL Studio, Cubase, Bitwig, Reaper, Studio One 등", "64비트 VST3를 지원하는 모든 DAW"]],
    "no_vst2": "<b>VST2는 왜 없나요?</b> Steinberg가 2018년부터 VST2 라이선스를 새로 내주지 않아서, 새 플러그인은 VST2로 "
               "배포할 수 없어요. 요즘 DAW는 모두 VST3를 쓰고, Logic·GarageBand는 AU를 써요.",

    "dl_title": "다운로드",
    "dl": "모든 파일은 릴리스 페이지에 있어요. 필요한 것만 받으면 돼요.",
    "dl_link": RELEASE,
    "dl_files": [["파일", "설명"],
                 ["StretchKit-mac-AU.zip", "맥 AU &mdash; Logic Pro, GarageBand"],
                 ["StretchKit-mac-VST3.zip", "맥 VST3 &mdash; Ableton, FL Studio, Cubase, Bitwig, Reaper 등"],
                 ["StretchKit-mac-Standalone.zip", "DAW 없이 실행하는 맥 앱"],
                 ["StretchKit-win-VST3.zip", "윈도우 VST3 플러그인"],
                 ["StretchKit-win-Standalone.zip", "DAW 없이 실행하는 윈도우 앱"]],

    "mac_title": "맥 설치",
    "mac_steps": [
        "받은 zip 파일을 더블클릭해서 압축을 풀어요.",
        "Finder에서 <b>이동 › 폴더로 이동...</b>을 누르고, 포맷에 맞는 폴더 경로를 붙여넣어요.",
    ],
    "mac_paths": [["포맷", "이 파일을", "이 폴더에 넣기"],
                  ["AU", "StretchKit.component", "~/Library/Audio/Plug-Ins/Components/"],
                  ["VST3", "StretchKit.vst3", "~/Library/Audio/Plug-Ins/VST3/"],
                  ["Standalone", "StretchKit.app", "/Applications/"]],
    "mac_unblock": "StretchKit은 무료라 애플 유료 개발자 인증을 받지 않았기 때문에, 인터넷에서 받은 파일을 맥이 막아요. "
                   "<b>터미널</b>(응용 프로그램 › 유틸리티)을 열고 설치한 포맷에 해당하는 줄을 실행해 주세요. "
                   "이 파일들에 붙은 '다운로드 표시'만 지우는 명령이에요.",
    "mac_cmds": "xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/StretchKit.component\n"
                "xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/StretchKit.vst3\n"
                "xattr -dr com.apple.quarantine /Applications/StretchKit.app",
    "mac_after": "DAW를 다시 켜요. Logic Pro에 그래도 안 보이면 아래 줄을 실행하고 Logic을 다시 켜요.",
    "mac_cmd2": "killall -9 AudioComponentRegistrar",

    "win_title": "윈도우 설치",
    "win_steps": [
        "받은 zip 파일을 오른쪽 클릭해서 <b>압축 풀기...</b>를 눌러요.",
        "<b>StretchKit.vst3</b> 폴더를 <font name='Courier'>C:\\Program Files\\Common Files\\VST3\\</font>에 복사해요. "
        "관리자 권한을 물어보면 <b>계속</b>을 눌러요.",
        "Standalone은 <b>StretchKit.exe</b>를 원하는 곳에 두면 돼요. 'Windows의 PC 보호' 창이 뜨면 "
        "<b>추가 정보 › 실행</b>을 눌러요. 서명이 없을 뿐 위험한 파일은 아니에요.",
        "DAW를 다시 켜고 새 플러그인을 스캔하게 해요.",
    ],
    "win_note": "따로 설치할 런타임은 없어요. 필요한 건 플러그인 안에 모두 들어 있어요.",

    "daw_title": "DAW에서 찾기",
    "daw_intro": "StretchKit은 <b>악기</b> 플러그인이라 MIDI(악기) 트랙에 올려요. 제작자 이름 <b>sxdrv.mp3</b> 아래에 있어요. "
                 "메뉴 이름은 DAW 버전에 따라 조금 다를 수 있어요.",
    "daw": [["DAW", "방법"],
            ["Ableton Live", "Settings › Plug-Ins에서 <i>Use VST3 Plug-in System Folders</i>를 켜고(맥은 <i>Use Audio Units</i>도) <b>Rescan</b>. Plug-Ins › sxdrv.mp3 아래에 있어요."],
            ["FL Studio", "Options › Manage plugins › <b>Find installed plugins</b> 후, 채널 랙(+)에서 추가해요."],
            ["Logic Pro / GarageBand", "AU 버전을 써요. 악기 슬롯 › AU Instruments › sxdrv.mp3 › StretchKit. 안 보이면 Settings › Plug-in Manager › Reset &amp; Rescan Selection."],
            ["Cubase / Nuendo", "켤 때 자동 스캔해요. 안 보이면 Studio › VST Plug-in Manager에서 확인해요."],
            ["Bitwig Studio", "Settings › Locations에서 VST3가 켜져 있는지 확인하고 다시 스캔해요."],
            ["Reaper", "Options › Preferences › Plug-ins › VST › <b>Re-scan</b>."],
            ["Studio One", "켤 때 자동 스캔해요. Options › Locations › VST Plug-ins에서 폴더 목록을 확인해요."]],

    "qs_title": "빠른 시작",
    "qs": [
        "악기 트랙에 StretchKit을 올려요.",
        "<b>Add Folder</b>를 누르고 드럼 샘플 폴더를 골라요. 스캔하는 동안 지도가 채워져요.",
        "처음 쓸 때는 라이브러리에서 무작위 스타터 킷을 골라 8개 트랙에 자동으로 넣어줘요.",
        "DAW의 재생(또는 StretchKit의 Play 버튼)을 누르면 기본 패턴이 재생돼요.",
        "지도 위를 클릭한 채 드래그하면서 소리를 들어보고, 그대로 지도 밖 트랙까지 끌고 가면 마지막에 들은 소리가 들어가요.",
        "트랙의 <b>LENGTH %</b>를 드래그했다가 놓으면, 늘이거나 줄인 새 소리가 바로 재생돼요. 트랙 이름을 클릭하면 다시 들을 수 있어요.",
    ],

    "ov_title": "화면 한눈에 보기",
    "ov": ["Add Folder / Folders / Rescan &mdash; 스캔할 폴더 관리",
           "Skin &mdash; 화면 디자인 바꾸기 (다음에도 기억)",
           "카테고리 필터 &mdash; 지도에서 종류별로 보이기/숨기기",
           "검색 / Favourites &mdash; 이름이나 하트로 지도 거르기",
           "샘플 지도 &mdash; 원샷 하나가 점 하나, 왼쪽 아래에 스캔 진행과 개수",
           "유사도 바 &mdash; 고른 소리와 가장 비슷한 24개",
           "히스토리, 하트, 선택한 트랙에 넣기",
           "Play / Stop &mdash; DAW가 멈춰 있을 때 시퀀서 재생",
           "Stretch 모드 &mdash; Hybrid, Smooth, Tape",
           "Drag MIDI &mdash; 패턴을 DAW로 끌어오기",
           "트랙 &mdash; 여기에 샘플을 놓아요. 클릭하면 선택하고 트랙 소리 그대로 들려줘요",
           "Rnd(주사위) &mdash; 비슷한 소리로 바꾸기, M &mdash; 뮤트",
           "VOL / PAN &mdash; 볼륨 / 좌우",
           "PITCH / LENGTH % &mdash; 음정과 타임스트레치",
           "스텝 &mdash; 이 트랙의 패턴"],

    "map_title": "샘플 지도",
    "map_text": [
        "<b>폴더 추가.</b> <b>Add Folder</b>를 누르고 폴더를 고르면 하위 폴더까지 스캔해요. 폴더를 빼려면 <b>Folders</b>, "
        "새 파일을 넣은 뒤에는 <b>Rescan</b>을 눌러요. 10초 이하의 WAV, AIFF, FLAC, OGG 파일이 들어가고, 더 긴 파일은 루프로 보고 "
        "건너뛰어요.",
        "<b>스캔은 한 번만.</b> 폴더와 분석 결과는 컴퓨터에 저장되고, 여러 StretchKit이 함께 써요. 다음에 열면 불러오기 없이 지도가 바로 떠요. "
        "샘플이 많으면 CPU 코어 여러 개로 나눠 분석하고 진행 상황도 중간중간 저장해요. 스캔 도중 플러그인이나 DAW를 꺼도, "
        "다음에 열 때 멈춘 곳부터 알아서 이어서 해요. Rescan은 새 파일이나 바뀐 파일만 확인해요.",
        "<b>분류 방법.</b> 파일과 폴더 이름을 먼저 봐요('kick', 'BD_01', 'Snares/...' 등). 이름에 단서가 없으면 소리 자체를 "
        "분석해요. 저음 비율, 밝기, 노이즈 정도, 감쇠 시간, 클랩 특유의 여러 번 터지는 소리 같은 걸로 판단해요. "
        "같은 종류 안에서는 비슷한 소리끼리 가깝게 놓여요.",
    ],
    "map_controls": [["이렇게 하면", "이렇게 돼요"],
                     ["점 클릭", "듣기, 그리고 유사도 바에 비슷한 소리 표시"],
                     ["클릭한 채 지도 위를 드래그", "지나가는 소리를 전부 듣기"],
                     ["그대로 지도 밖 트랙까지 드래그", "마지막에 들은 소리를 그 트랙에 넣기"],
                     ["점 더블클릭", "선택된 트랙에 넣기"],
                     ["마우스 휠 / 트랙패드 핀치", "확대·축소"],
                     ["두 손가락 스크롤, Cmd+드래그, 우클릭 드래그", "이동 (빈 곳 더블클릭하면 원래대로)"],
                     ["검색창 / Favourites", "이름에 단어가 들어간 소리만 / 하트 누른 소리만 보기"],
                     ["위쪽 카테고리 버튼", "그 종류 보이기/숨기기"]],
    "map_drop": "Finder나 탐색기에서 오디오 파일을 트랙으로 바로 끌어다 놓아도 돼요.",

    "sim_title": "유사도 바",
    "sim_text": "지도 아래 띠에는 고른 소리(큰 원)와, 그와 가장 비슷한 24개 소리가 가까운 순서대로 나와요. 같은 점들이 지도에서도 표시돼요.",
    "sim_controls": [["컨트롤", "기능"],
                     ["|&lt;  &lt;  &gt;", "고른 소리로 돌아가기 / 이전 / 비슷한 소리를 차례로 재생 (다시 누르면 멈춤)"],
                     ["점", "클릭하면 듣기, 트랙으로 끌어다 놓으면 넣기"],
                     ["History &#9650; &#9660;", "전에 골랐던 소리로 앞뒤 이동"],
                     ["하트", "즐겨찾기 표시(저장됨). 지도 위 <b>Favourites</b>를 켜면 이것만 보여요"],
                     ["넣기 버튼", "선택한 트랙에 넣고 재생"]],

    "st_title": "드럼 타임스트레치",
    "st_text": "<b>LENGTH %</b>는 각 트랙의 길이를 <b>원본 샘플</b> 기준으로 정해요. 50%는 절반, 200%는 두 배 길이예요"
               "(25~400%, 더블클릭하면 100%). <b>PITCH</b>는 반음 단위로 음정을 바꿔요(&plusmn;24). 음정을 바꿔도 LENGTH %는 "
               "원본 길이 기준 그대로라서 음정과 길이를 따로 조절할 수 있어요.",
    "st_modes": [["모드", "동작", "추천"],
                 ["Hybrid (기본)", "어택은 녹음된 그대로 두고 꼬리만 늘이거나 줄여요.", "킥, 스네어, 클랩 등 펀치감 있는 소리"],
                 ["Smooth", "샘플 전체를 Rubber Band R3로 처리해요.", "길거나 음정·질감이 있는 소리"],
                 ["Tape", "속도를 바꿔요. 길게 하면 낮아지고 짧게 하면 높아져요(느려진 레코드처럼).", "로파이, 피치다운 효과"]],
    "st_note": "스트레치는 샘플이나 설정이 바뀔 때 백그라운드에서 미리 계산해 둬서, 재생할 때는 CPU를 거의 쓰지 않고 지연도 없어요. "
               "PITCH나 LENGTH %에서 손을 떼면 트랙이 새 소리를 한 번 재생하고, 트랙 이름을 클릭하면 다시 들을 수 있어요. "
               "참고로 <b>Choke</b>가 켜져 있으면 같은 트랙의 다음 타격이 긴 꼬리를 잘라요. 패턴에서 긴 소리를 들으려면 Choke를 꺼요.",

    "seq_title": "스텝 시퀀서",
    "seq_steps": [["이렇게 하면", "이렇게 돼요"],
                  ["스텝 클릭", "켜기 / 끄기"],
                  ["우클릭 또는 Shift+클릭", "액센트(더 크게, 하이라이트 표시)"],
                  ["클릭한 채 여러 칸 드래그", "한 번에 여러 칸 입력"]],
    "seq_transport": [["컨트롤", "기능"],
                      ["Play / Stop", "DAW가 멈춰 있을 때 시퀀서를 재생해요. DAW를 재생하면 그쪽을 따라가요(버튼에 <i>Host</i> 표시)."],
                      ["BPM", "내부 클럭 템포. DAW가 템포를 주면 DAW 템포를 써요."],
                      ["Steps / Rate", "패턴 길이(8, 16, 32)와 한 칸 길이(1/8, 1/16, 1/32, 1/16 셋잇단)."],
                      ["Swing", "두 번째 칸마다 살짝 늦춰서 셔플 느낌을 줘요."],
                      ["Choke", "같은 트랙에서 새로 칠 때 이전 소리를 짧게 줄여요. 끄면 길게 늘인 꼬리가 겹쳐 울려요."],
                      ["Master", "전체 출력 볼륨."],
                      ["Drag MIDI", "DAW의 MIDI 트랙으로 끌어다 놓으면 패턴이 클립으로 들어가요."]],
    "seq_midi": "<b>MIDI로 연주하기.</b> 노트 36~43이 트랙 1~8을 연주해요(대부분 DAW에서 C1~G1, FL Studio 등 일부는 같은 노트를 "
                "C3~G3로 표시해요). 벨로시티가 세기를 정해요.",

    "sk_title": "스킨",
    "sk_text": "위쪽 <b>SKIN</b> 메뉴에서 디자인을 고를 수 있어요. 마지막으로 고른 스킨이 새로 여는 플러그인의 기본값이 되고, "
               "프로젝트마다 고른 스킨도 함께 저장돼요.",
    "skins": [("skin_A_ps2_night.png", "PS2 Night", "짙은 남색, 안개, 유리 타워와 메뉴 불빛."),
              ("skin_B_y2k_chrome.png", "Y2K Chrome", "펄 실버, 크롬 캡슐 버튼, 젤리 스텝."),
              ("skin_C_digicore.png", "Digicore", "검정과 네온, 픽셀 폰트, 글리치, 스캔라인."),
              ("skin_D_mix.png", "Mix", "PS2 바탕에 Y2K 크롬과 디지코어 반짝이."),
              ("skin_E_liquid_chrome.png", "Liquid Chrome", "블루-바이올렛 유리와 3D 리퀴드 크롬 아이콘.")],

    "ts_title": "문제 해결",
    "ts": [("맥에서 '손상되었거나 열 수 없다'고 나와요.", "'맥 설치'의 터미널 명령을 실행하고 DAW를 다시 켜요."),
           ("DAW에서 StretchKit이 안 보여요.", "설치 폴더(3·4장)를 확인하고, VST3/AU 스캔이 켜져 있는지 본 뒤 다시 스캔해요. 악기 플러그인이라 악기 목록에서 찾아야 해요."),
           ("소리가 안 나요.", "DAW나 StretchKit의 Play를 눌렀는지, 트랙에 샘플이 있고 뮤트(M)가 꺼져 있는지, 스텝이 켜져 있는지 확인해요."),
           ("지도가 비어 있어요.", "Add Folder로 폴더를 추가해요. 10초 이하의 WAV/AIFF/FLAC/OGG 원샷만 들어가요. 스캔 진행 상황은 지도 왼쪽 아래에 보여요."),
           ("스캔 중에 플러그인을 꺼버렸어요.", "괜찮아요. 다음에 StretchKit을 열면 멈춘 곳부터 알아서 이어서 스캔해요."),
           ("LENGTH나 PITCH를 바꿔도 차이가 없어요.", "트랙 이름을 클릭하면 그 트랙 소리 그대로 들려줘요. 패턴에서는 Choke가 다음 타격 때 긴 꼬리를 자르니, 길게 들으려면 Choke를 꺼요."),
           ("트랙에 '(missing)'이 떠요.", "파일을 옮기거나 이름을 바꾼 경우예요. 지도에서 다시 넣거나 Rescan을 눌러요."),
           ("소리가 다른 카테고리에 들어가 있어요.", "이름이 우선이라, 파일이나 폴더 이름에 'kick' 같은 단어를 넣고 Rescan하면 고쳐져요."),
           ("윈도우에서 Standalone 앱에 경고가 떠요.", "추가 정보 › 실행을 눌러요. 무료 플러그인은 보통 서명이 없어서 그래요.")],

    "lic_title": "라이선스 &amp; 크레딧",
    "lic": [
        "StretchKit은 <b>GNU GPL v3</b> 라이선스의 무료 오픈소스예요. 소스 코드: " + REPO,
        "<b>JUCE 8</b>(AGPLv3 조건으로 사용)과 Particular Programs Ltd.의 <b>Rubber Band Library</b>(GPLv2 이상)로 만들었어요. "
        "폰트: Rajdhani, Michroma, VT323, Silkscreen, Press Start 2P, Inter (SIL Open Font License).",
        "VST는 Steinberg Media Technologies GmbH의 상표이고, Audio Units와 macOS는 Apple Inc.의 상표예요.",
    ],
    "credit_line": "Made by",
}
