#pragma once

// Forwarding header — FFragment_SaveKey moved to CkEcs so lower-tier modules can stamp keys.
// Kept for out-of-plugin consumers (CkGameplayDebugger, older game code); new code includes the
// CkEcs path directly.
#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"
