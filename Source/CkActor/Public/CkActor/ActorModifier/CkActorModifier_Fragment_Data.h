#pragma once

#include "CkCore/Actor/CkActor.h"
#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Format/CkFormat.h"

#include "CkActorModifier_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SpawnActor_PostSpawnPolicy : uint8
{
    None,
    AttachImmediately
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SpawnActor_PostSpawnPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SpawnActor_SpawnPolicy : uint8
{
    SpawnOnInstanceWithOwership,
    SpawnOnServer,
    SpawnOnAll
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SpawnActor_SpawnPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ActorComponent_AttachmentPolicy : uint8
{
    /* Attach to owner/parent and transform with them */
    Attach,
    /* Do not attach - component will stay where it spawned */
    DoNotAttach
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ActorComponent_AttachmentPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_SpawnActor_PostSpawn_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SpawnActor_PostSpawn_Params);

public:
    FCk_SpawnActor_PostSpawn_Params() = default;
    explicit FCk_SpawnActor_PostSpawn_Params(
        FCk_Actor_AttachToActor_Params InParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_SpawnActor_PostSpawnPolicy _PostSpawnPolicy = ECk_SpawnActor_PostSpawnPolicy::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_PostSpawnPolicy != ECk_SpawnActor_PostSpawnPolicy::None"))
    FCk_Actor_AttachToActor_Params _Params;

public:
    CK_PROPERTY_GET(_PostSpawnPolicy);
    CK_PROPERTY_GET(_Params);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_AddActorComponent_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AddActorComponent_Params);

    FCk_AddActorComponent_Params() = default;
    explicit FCk_AddActorComponent_Params(
        USceneComponent* InParent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess = true, EditCondition = "_AttachmentType == ECk_ActorComponent_AttachmentPolicy::Attach"))
    TObjectPtr<USceneComponent> _Parent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _IsTickEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_IsTickEnabled"))
    FCk_Time _TickInterval = FCk_Time::ZeroSecond();

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ActorComponent_AttachmentPolicy _AttachmentType = ECk_ActorComponent_AttachmentPolicy::Attach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess = true, EditCondition = "_AttachmentType == ECk_ActorComponent_AttachmentPolicy::Attach"))
    FName _AttachmentSocket;

public:
    CK_PROPERTY_GET(_Parent);
    CK_PROPERTY(_IsTickEnabled);
    CK_PROPERTY(_TickInterval);
    CK_PROPERTY(_AttachmentType);
    CK_PROPERTY(_AttachmentSocket);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_SpawnActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_SpawnActor);

public:
    using PreFinishSpawnFuncType = TFunction<void(AActor*)>;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Utils_Actor_SpawnActor_Params _SpawnParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_SpawnActor_PostSpawn_Params _PostSpawnParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_SpawnActor_SpawnPolicy _SpawnPolicy = ECk_SpawnActor_SpawnPolicy::SpawnOnInstanceWithOwership;

private:
    PreFinishSpawnFuncType _PreFinishSpawnFunc = nullptr;

public:
    CK_PROPERTY_GET(_SpawnParams);
    CK_PROPERTY(_PostSpawnParams);
    CK_PROPERTY(_PreFinishSpawnFunc);
    CK_PROPERTY(_SpawnPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ActorModifier_SpawnActor, _SpawnParams);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ActorModifier_OnActorSpawned,
    AActor*, InActorSpawned,
    const FInstancedStruct&, InOptionalPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ActorModifier_OnActorSpawned_MC,
    AActor*, InActorSpawned,
    const FInstancedStruct&, InOptionalPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_AddActorComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_AddActorComponent);

public:
    using InitializerFuncType = TFunction<void(UActorComponent*)>;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UActorComponent> _ComponentToAdd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _IsUnique = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AddActorComponent_Params _ComponentParams;

private:
    InitializerFuncType _InitializerFunc = nullptr;

public:
    CK_PROPERTY_GET(_ComponentToAdd);
    CK_PROPERTY(_IsUnique);
    CK_PROPERTY(_ComponentParams);
    CK_PROPERTY(_InitializerFunc);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ActorModifier_AddActorComponent, _ComponentToAdd);
};

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_ActorModifier_OnActorComponentAdded,
    AActor*, InOwner,
    UActorComponent*, InActorComponentAdded,
    const FInstancedStruct&, InOptionalPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_ActorModifier_OnActorComponentAdded_MC,
    AActor*, InOwner,
    UActorComponent*, InActorComponentAdded,
    const FInstancedStruct&, InOptionalPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_AttachActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_AttachActor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Actor_AttachToActor_Params _Params;

public:
    CK_PROPERTY_GET(_Params);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ActorModifier_AttachActor, _Params);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_ActorModifier_OnActorAttached,
    FCk_Handle, InHandle,
    AActor*, InActorAttached,
    AActor*, InActorAttachedTo);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_ActorModifier_OnActorAttached_MC,
    FCk_Handle, InHandle,
    AActor*, InActorAttached,
    AActor*, InActorAttachedTo);

// --------------------------------------------------------------------------------------------------------------------
