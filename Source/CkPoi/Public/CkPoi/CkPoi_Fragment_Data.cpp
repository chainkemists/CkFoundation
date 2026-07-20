#include "CkPoi_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

// Parent root for all POI category tags. The "Poi.Category" picker root itself is declared extern in
// CkPoi_Utils.h and defined in CkPoi_Utils.cpp (Timer precedent: Tag_Timer_CategoryName).
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Poi_Root, TEXT("Poi"));

// --------------------------------------------------------------------------------------------------------------------
