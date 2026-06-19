#pragma once
#include "CoreMinimal.h"

namespace ck::pmg
{
    // Task 1 link smoke: initializes and tears down a transient FT_Library.
    CKPMG_API auto FontGlyph_SelfTest_FreeTypeInitDone() -> bool;
}
