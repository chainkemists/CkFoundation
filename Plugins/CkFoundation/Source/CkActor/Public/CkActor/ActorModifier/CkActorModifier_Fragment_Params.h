#pragma once

#include "CkCore/Actor/CkActor.h"
#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkFormat/CkFormat.h"

#include "CkActorModifier_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SpawnActor_PostSpawnPolicy : uint8
{
    None,
    AttachImmediately
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SpawnActor_PostSpawnPolicy);

UENUM(BlueprintType)
enum class ECk_RelativeAbsolute : uint8
{
    Relative,
    Absolute
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RelativeAbsolute);

UENUM(BlueprintType)
enum class ECk_LocalWorld : uint8
{
    Local,
    World
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_LocalWorld);

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
    FCk_SpawnActor_PostSpawn_Params(
        ECk_SpawnActor_PostSpawnPolicy InPostSpawnPolicy,
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
    CK_GENERATED_BODY(FCk_SpawnActor_PostSpawn_Params);

public:
    FCk_AddActorComponent_Params() = default;
    FCk_AddActorComponent_Params(
        bool InIsTickEnabled,
        FCk_Time InTickInterval,
        ECk_ActorComponent_AttachmentPolicy InAttachmentType,
        USceneComponent* InParent,
        FName InAttachmentSocket);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _IsTickEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_IsTickEnabled"))
    FCk_Time _TickInterval = FCk_Time::Zero;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ActorComponent_AttachmentPolicy _AttachmentType = ECk_ActorComponent_AttachmentPolicy::Attach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess = true, EditCondition = "_AttachmentType == ECk_ActorComponent_AttachmentPolicy::Attach"))
    TObjectPtr<USceneComponent> _Parent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess = true, EditCondition = "_AttachmentType == ECk_ActorComponent_AttachmentPolicy::Attach"))
    FName _AttachmentSocket;

public:
    CK_PROPERTY_GET(_IsTickEnabled);
    CK_PROPERTY_GET(_TickInterval);
    CK_PROPERTY_GET(_AttachmentType);
    CK_PROPERTY_GET(_Parent);
    CK_PROPERTY_GET(_AttachmentSocket);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_SetLocation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_SetLocation);

public:
    FCk_Request_ActorModifier_SetLocation() = default;
    FCk_Request_ActorModifier_SetLocation(
        FVector              InNewLocation,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NewLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewLocation)
    CK_PROPERTY_GET(_RelativeAbsolute)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_AddLocationOffset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_AddLocationOffset);

public:
    FCk_Request_ActorModifier_AddLocationOffset() = default;
    FCk_Request_ActorModifier_AddLocationOffset(
        FVector        InDeltaLocation,
        ECk_LocalWorld InLocalWorld);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _DeltaLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_DeltaLocation)
    CK_PROPERTY_GET(_LocalWorld)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_SetScale
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_SetScale);

public:
    FCk_Request_ActorModifier_SetScale() = default;
    FCk_Request_ActorModifier_SetScale(
        FVector              InNewScale,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NewScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewScale)
    CK_PROPERTY_GET(_RelativeAbsolute)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_SetRotation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_SetRotation);

public:
    FCk_Request_ActorModifier_SetRotation() = default;
    FCk_Request_ActorModifier_SetRotation(
        FRotator             InNewRotation,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _NewRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewRotation)
    CK_PROPERTY_GET(_RelativeAbsolute)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_AddRotationOffset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_AddRotationOffset);

public:
    FCk_Request_ActorModifier_AddRotationOffset() = default;
    FCk_Request_ActorModifier_AddRotationOffset(
        FRotator       InDeltaRotation,
        ECk_LocalWorld InLocalWorld);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _DeltaRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_DeltaRotation)
    CK_PROPERTY_GET(_LocalWorld)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_SetTransform
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_SetTransform);

public:
    FCk_Request_ActorModifier_SetTransform() = default;
    FCk_Request_ActorModifier_SetTransform(
        FTransform           InNewTransform,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _NewTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewTransform)
    CK_PROPERTY_GET(_RelativeAbsolute)
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

public:
    FCk_Request_ActorModifier_SpawnActor() = default;
    FCk_Request_ActorModifier_SpawnActor(
        FCk_Utils_Actor_SpawnActor_Params InSpawnParams,
        FCk_SpawnActor_PostSpawn_Params InPostSpawnParams,
        PreFinishSpawnFuncType InPreFinishSpawnFunc = nullptr);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Utils_Actor_SpawnActor_Params _SpawnParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_SpawnActor_PostSpawn_Params _PostSpawnParams;

private:
    PreFinishSpawnFuncType _PreFinishSpawnFunc = nullptr;

public:
    CK_PROPERTY_GET(_SpawnParams);
    CK_PROPERTY_GET(_PostSpawnParams);
    CK_PROPERTY_GET(_PreFinishSpawnFunc);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ActorModifier_OnActorSpawned,
    FCk_Handle, InHandle,
    AActor*, InActorSpawned);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ActorModifier_OnActorSpawned_MC,
    FCk_Handle, InHandle,
    AActor*, InActorSpawned);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_AddActorComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_AddActorComponent);

public:
    using InitializerFuncType = TFunction<void(UActorComponent*)>;

public:
    FCk_Request_ActorModifier_AddActorComponent() = default;
    FCk_Request_ActorModifier_AddActorComponent(
        TSubclassOf<UActorComponent> InComponentToAdd,
        bool InIsUnique,
        FCk_AddActorComponent_Params InComponentParams,
        InitializerFuncType InInitializerFunc = nullptr);

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
    CK_PROPERTY_GET(_IsUnique);
    CK_PROPERTY_GET(_ComponentParams);
    CK_PROPERTY_GET(_InitializerFunc);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ActorModifier_OnActorComponentAdded,
    FCk_Handle, InHandle,
    UActorComponent*, InActorComponentAdded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ActorModifier_OnActorComponentAdded_MC,
    FCk_Handle, InHandle,
    UActorComponent*, InActorComponentAdded);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Request_ActorModifier_AttachActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorModifier_AttachActor);

public:
    FCk_Request_ActorModifier_AttachActor() = default;
    explicit FCk_Request_ActorModifier_AttachActor(
        FCk_Actor_AttachToActor_Params InParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Actor_AttachToActor_Params _Params;

public:
    CK_PROPERTY_GET(_Params);
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

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Transform_OnUpdate,
    FCk_Handle, InHandle,
    FTransform, InNewTransform,
    FTransform, InPreviousTransform);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_Transform_OnUpdate_MC,
    FCk_Handle, InHandle,
    FTransform, InNewTransform,
    FTransform, InPreviousTransform);

// --------------------------------------------------------------------------------------------------------------------
