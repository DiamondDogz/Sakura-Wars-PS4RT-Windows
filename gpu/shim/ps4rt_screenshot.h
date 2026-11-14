// SPDX-License-Identifier: GPL-2.0-or-later
// F12 screenshots of the game's own image (without the overlay), written by the presenter to the
// user folder's gpu/screenshots/. The capture itself follows the Valkyria Revolution port.
#pragma once

#include <string>

namespace Ps4RtScreenshot {

/// Any thread: save the next presented game frame (F12).
void Request();

/// Any thread: save the next presented game frame as `<stem>.png` (Crowbar asks for one when a
/// trophy unlocks, crowbar-sdk 1.6). The stem is letters, digits, '-' and '_' only.
void RequestNamed(const char* stem);

/// Present thread: the number of screenshots requested since the last call.
unsigned Consume();

/// Present thread, after Consume(): the name asked with RequestNamed (empty for F12).
std::string TakeName();

} // namespace Ps4RtScreenshot
