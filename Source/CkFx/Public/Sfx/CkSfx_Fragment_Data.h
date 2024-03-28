#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>

#include "CkSfx_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKFX_API FCk_Handle_Sfx : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Sfx); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Sfx);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Fragment_Sfx_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Sfx_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USoundBase> _SoundCue;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_SoundCue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Fragment_MultipleSfx_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleSfx_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_Sfx_ParamsData> _SfxParams;

public:
    CK_PROPERTY_GET(_SfxParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleSfx_ParamsData, _SfxParams);
};

// --------------------------------------------------------------------------------------------------------------------