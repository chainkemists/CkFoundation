#pragma once

#include "Stats/Stats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkFx"), STATGROUP_CkFx, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------
// Shared across the Vfx and Sfx spawn translation units — one counter, defined once (see CkVfx_Utils.cpp).

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Fx Effects Spawned"), STAT_Fx_EffectsSpawned, STATGROUP_CkFx, CKFX_API);

// --------------------------------------------------------------------------------------------------------------------
