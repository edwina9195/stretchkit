# StretchKit — licence

Copyright (C) 2026 sxdrv.mp3 (https://www.instagram.com/sxdrv.mp3/)

StretchKit's own source code is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version. The full text is in `LICENSE`.

It is built together with JUCE, which is used under the GNU Affero General Public License v3.
Section 13 of both licences explicitly allows this combination; the JUCE parts stay under AGPLv3.
In practice: anyone who gets a StretchKit binary must also be able to get this source code,
which this public repository already provides.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

## Third-party components

| Component | Licence | Notes |
|---|---|---|
| JUCE 8 (https://github.com/juce-framework/JUCE) | AGPLv3 (used under these terms) | Fetched at build time |
| Rubber Band Library v4.0.0 (https://breakfastquay.com/rubberband/) | GPLv2 or later | Copyright Particular Programs Ltd. Fetched at build time, compiled from `single/RubberBandSingle.cpp` |
| VST3 SDK (bundled with JUCE) | MIT | VST is a trademark of Steinberg Media Technologies GmbH |
| Fonts: Rajdhani, Michroma, VT323, Silkscreen, Press Start 2P, Inter | SIL Open Font License 1.1 | Embedded in the plugin; licence texts in `Resources/fonts/licenses/` |

Because Rubber Band is GPL, a closed-source build of StretchKit would need a commercial
Rubber Band licence. As distributed here (source available under GPLv3), no extra licence
is required.
