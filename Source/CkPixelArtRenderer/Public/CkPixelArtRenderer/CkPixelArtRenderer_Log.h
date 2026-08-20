#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// --------------------------------------------------------------------------------------------------------------------

// A plain UE log category rather than the house CK_DEFINE_LOG_FUNCTIONS: that macro lives in CkLog, and this module
// loads at PostConfigInit where no Ck module may be linked (see CkPixelArtRender.Build.cs). Grep breadcrumbs by
// category name.
CKPIXELARTRENDER_API DECLARE_LOG_CATEGORY_EXTERN(LogCkPixelArt, Log, All);
