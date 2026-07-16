#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkAStar_Test_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKASTAR_API FCk_Handle_AStarTest : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AStarTest);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AStarTest);

// --------------------------------------------------------------------------------------------------------------------
