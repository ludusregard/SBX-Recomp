// Our own audio output, replacing the SDK's render driver and Audio Worker
// (ported from ITC-Recomp, src/audio/native_audio.h; its docs/native-audio.md).

#pragma once

#include <rex/runtime.h>

namespace native_audio {

// Puts our audio system in the runtime's config, unless --native_audio is
// off. Called from SbxApp::OnPreSetup.
void Configure(rex::RuntimeConfig& config);

}  // namespace native_audio
