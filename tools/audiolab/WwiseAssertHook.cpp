// Copyright © 2026 CCP ehf.

// The Profile build of AkAutobahn asserts through this hook, which AkSoundEngine defines inside _audio2 only.
// Null turns those asserts off.
extern "C" void ( *g_pAssertHook )( const char*, const char*, int ) = nullptr;
