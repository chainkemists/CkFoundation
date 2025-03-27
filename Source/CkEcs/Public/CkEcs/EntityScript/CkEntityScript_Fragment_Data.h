#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEntityScript_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_EntityScript_SpawnEntity
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityScript_SpawnEntity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _NewEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Owner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _EntityScriptClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_EntityScript_UE> _EntityScriptTemplate;

public:
    CK_PROPERTY_GET(_NewEntity);
    CK_PROPERTY_GET(_EntityScriptClass);
    CK_PROPERTY_GET(_Owner);

    CK_PROPERTY(_EntityScriptTemplate);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityScript_SpawnEntity, _NewEntity, _Owner, _EntityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_EntityScript_Replicate
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityScript_Replicate);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Owner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _EntityScriptClass;

public:
    CK_PROPERTY_GET(_EntityScriptClass);
    CK_PROPERTY_GET(_Owner);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityScript_Replicate, _Owner, _EntityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------
