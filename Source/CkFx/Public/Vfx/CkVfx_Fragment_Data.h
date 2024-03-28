#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>
#include <NiagaraSystem.h>

#include "CkVfx_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKFX_API FCk_Handle_Vfx : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Vfx); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Vfx);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Fragment_Vfx_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Vfx_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UNiagaraSystem> _ParticleSystem;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_ParticleSystem);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Fragment_MultipleVfx_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleVfx_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Vfx_ParamsData> _VfxParams;

public:
    CK_PROPERTY_GET(_VfxParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleVfx_ParamsData, _VfxParams);
};

// --------------------------------------------------------------------------------------------------------------------