#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkVfxCue_Fragment_Data.h"

#include "CkVfxCue_DataChannel_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVFX_API FCk_Handle_VfxCue_DataChannel : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VfxCue_DataChannel); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VfxCue_DataChannel);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVFX_API FCk_VfxCue_DataChannel_SpawnParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VfxCue_DataChannel_SpawnParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true))
    FVector _SearchLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_VfxCue_UserParameter> _Parameters;

public:
    CK_PROPERTY(_SearchLocation);
    CK_PROPERTY(_Parameters);

    CK_DEFINE_CONSTRUCTORS(FCk_VfxCue_DataChannel_SpawnParams, _SearchLocation);
};

// --------------------------------------------------------------------------------------------------------------------
