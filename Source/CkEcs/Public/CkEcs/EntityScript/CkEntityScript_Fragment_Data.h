#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkEntityScript_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKECS_API FCk_Handle_EntityScript : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityScript); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityScript);

// --------------------------------------------------------------------------------------------------------------------

using FCk_EntityScript_PostConstruction_Func = std::function<void(FCk_Handle)>;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKECS_API FCk_Handle_PendingEntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_PendingEntityScript);

private:
    UPROPERTY()
    FCk_Handle _EntityUnderConstruction;

public:
    CK_PROPERTY_GET(_EntityUnderConstruction);

    CK_DEFINE_CONSTRUCTORS(FCk_Handle_PendingEntityScript, _EntityUnderConstruction);
};

CK_DECLARE_CUSTOM_IS_VALID(CKECS_API, FCk_Handle_PendingEntityScript, IsValid_Policy_Default);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_EntityScript_SpawnEntity : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityScript_SpawnEntity);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityScript_SpawnEntity);

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
    FInstancedStruct _SpawnParams;

private:
    FCk_EntityScript_PostConstruction_Func _PostConstruction_Func;

public:
    CK_PROPERTY_GET(_NewEntity);
    CK_PROPERTY_GET(_EntityScriptClass);
    CK_PROPERTY_GET(_Owner);

    CK_PROPERTY(_SpawnParams);
    CK_PROPERTY(_PostConstruction_Func);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityScript_SpawnEntity, _NewEntity, _Owner, _EntityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_EntityScript_Replicate : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityScript_Replicate);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityScript_Replicate);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Owner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _EntityScriptClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FInstancedStruct _SpawnParams;

private:
    FCk_EntityScript_PostConstruction_Func _PostReplicationDriver_Func;

public:
    CK_PROPERTY_GET(_EntityScriptClass);
    CK_PROPERTY_GET(_Owner);
    CK_PROPERTY_GET(_SpawnParams);
    CK_PROPERTY(_PostReplicationDriver_Func);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityScript_Replicate, _Owner, _EntityScriptClass, _SpawnParams);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_EntityScript_Constructed,
    FCk_Handle_EntityScript, InEntityScriptHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_EntityScript_Constructed_MC,
    FCk_Handle_EntityScript, InEntityScriptHandle);

// --------------------------------------------------------------------------------------------------------------------
