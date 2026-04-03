#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEntityScript_WithActor_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_EntityScript_WithActor_SpawnParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityScript_WithActor_SpawnParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<AActor> _OwningActor;

public:
    CK_PROPERTY_GET(_OwningActor);

    CK_DEFINE_CONSTRUCTORS(FCk_EntityScript_WithActor_SpawnParams, _OwningActor);
};

// --------------------------------------------------------------------------------------------------------------------
