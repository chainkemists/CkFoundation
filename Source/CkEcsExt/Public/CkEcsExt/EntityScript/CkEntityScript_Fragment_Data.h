#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/EntityScript/CkEntityScript.h"

#include "CkEntityScript_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Request_EntityScript_SpawnEntity
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityScript_SpawnEntity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_PDA> _EntityScriptClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_EntityScript_PDA> _EntityScriptTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Owner;

public:
    CK_PROPERTY_GET(_EntityScriptClass);
    CK_PROPERTY(_EntityScriptTemplate);
    CK_PROPERTY(_Owner);

CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityScript_SpawnEntity, _EntityScriptClass);
};