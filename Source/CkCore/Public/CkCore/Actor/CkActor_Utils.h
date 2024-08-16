#pragma once

#include "CkActor.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include <CoreMinimal.h>
#include <Components/MeshComponent.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkActor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Utils_Actor_SpawnActorPolicy : uint8
{
    Default,
    CannotSpawnInPersistentLevel
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Utils_Actor_SpawnActorPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Actor_SpawnActor_Params
{
    GENERATED_BODY()

    friend class UCk_Utils_Actor_UE;

public:
    CK_GENERATED_BODY(FCk_Utils_Actor_SpawnActor_Params);

public:
    FCk_Utils_Actor_SpawnActor_Params() = default;
    FCk_Utils_Actor_SpawnActor_Params(
        UObject* InOwnerOrWorld,
        TSubclassOf<AActor> InActorClass);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<UObject> _OwnerOrWorld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> _ActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> _Archetype;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName _NonUniqueName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString _Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform _SpawnTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESpawnActorCollisionHandlingMethod _CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECk_Actor_NetworkingType _NetworkingType = ECk_Actor_NetworkingType::Local;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECk_Utils_Actor_SpawnActorPolicy _SpawnPolicy = ECk_Utils_Actor_SpawnActorPolicy::Default;

public:
    CK_PROPERTY_GET(_OwnerOrWorld);
    CK_PROPERTY_GET(_ActorClass);

    CK_PROPERTY(_Archetype);
    CK_PROPERTY(_NonUniqueName);
    CK_PROPERTY(_Label);
    CK_PROPERTY(_SpawnTransform);
    CK_PROPERTY(_CollisionHandlingOverride);
    CK_PROPERTY(_NetworkingType);
    CK_PROPERTY(_SpawnPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Actor_SocketTransforms
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_Actor_SocketTransforms);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess))
    TWeakObjectPtr<UMeshComponent> _ComponentWithSocket = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess))
    FTransform _Transform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess))
    FName _Name;

public:
    CK_PROPERTY_GET(_ComponentWithSocket);
    CK_PROPERTY_GET(_Transform);
    CK_PROPERTY_GET(_Name);

    CK_DEFINE_CONSTRUCTORS(FCk_Utils_Actor_SocketTransforms, _ComponentWithSocket, _Transform, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Actor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Actor_UE);

public:
    using SpawnActorParamsType = FCk_Utils_Actor_SpawnActor_Params;

public:
    template <typename T_CompType>
    struct AddNewActorComponent_Params
    {
        CK_GENERATED_BODY(AddNewActorComponent_Params);

    public:
        using CompClassType = TSubclassOf<T_CompType>;
        using CompType = T_CompType;

    public:
        explicit
        AddNewActorComponent_Params(
            AActor* InTargetActor);

        AddNewActorComponent_Params(
            AActor* InTargetActor,
            USceneComponent* InParent);

        AddNewActorComponent_Params(
            AActor* InTargetActor,
            CompClassType InCompClass,
            USceneComponent* InParent);

    private:
        TWeakObjectPtr<AActor> _Owner;
        TWeakObjectPtr<USceneComponent> _ParentComp;
        CompClassType _CompClass;
        bool _IsUnique = true;
        FName _Socket;
        TArray<FName> _Tags;

    public:
        CK_PROPERTY_GET(_Owner);
        CK_PROPERTY_GET(_ParentComp);
        CK_PROPERTY_GET(_CompClass);
        CK_PROPERTY(_IsUnique);
        CK_PROPERTY(_Socket);
        CK_PROPERTY(_Tags);
    };

private:
    struct DeferredSpawnActor_Params
    {
        CK_GENERATED_BODY(DeferredSpawnActor_Params);

    public:
        DeferredSpawnActor_Params(
            TSubclassOf<AActor> InActorClass,
            UObject* InOwnerOrWorld);

    private:
        TSubclassOf<AActor> _ActorClass;
        TWeakObjectPtr<UObject> _OwnerOrWorld;
        TWeakObjectPtr<AActor> _Archetype;
        FName _NonUniqueName;
        FString _Label;
        FTransform _SpawnTransform;
        ESpawnActorCollisionHandlingMethod _CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;

    public:
        CK_PROPERTY_GET(_ActorClass);
        CK_PROPERTY_GET(_OwnerOrWorld);
        CK_PROPERTY(_Archetype);
        CK_PROPERTY(_NonUniqueName);
        CK_PROPERTY(_Label);
        CK_PROPERTY(_SpawnTransform);
        CK_PROPERTY(_CollisionHandlingOverride);
    };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Persistent Level Script Actor")
    static ALevelScriptActor*
    Get_PersistentLevelScriptActor(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Outermost Pawn",
              meta = (DefaultToSelf = "InObject"))
    static APawn*
    Get_OutermostPawn(
        UObject* InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Outermost Actor",
              meta = (DefaultToSelf = "InObject"))
    static AActor*
    Get_OutermostActor(
        UObject* InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Outermost Actor With Remote Authority",
              meta = (DefaultToSelf = "InObject"))
    static AActor*
    Get_OutermostActor_RemoteAuthority(
        UObject* InObject);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Request Clone Actor",
              meta = (DefaultToSelf = "InOwner"))
    static AActor*
    Request_CloneActor(
        AActor* InWorldContextActor,
        AActor* InOwner,
        AActor* InActorToClone,
        ESpawnActorCollisionHandlingMethod InCollisionHandlingOverride,
        ECk_Utils_Actor_SpawnActorPolicy InSpawnPolicy);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Has Component By Class",
              meta = (DefaultToSelf = "InActor"))
    static bool
    Get_HasComponentByClass(
        AActor* InActor,
        TSubclassOf<UActorComponent> InComponent);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Does Bone Exist In Skeletal Mesh",
              meta = (DefaultToSelf = "InActor"))
    static bool
    Get_DoesBoneExistInSkeletalMesh(
        AActor* InActor,
        FName InBoneName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Socket Transform",
              meta = (DefaultToSelf = "InActor"))
    static TArray<FCk_Utils_Actor_SocketTransforms>
    Get_SocketTransform(
        AActor* InActor,
        FName InSocketName,
        ERelativeTransformSpace InTransformSpace = RTS_World);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Socket Transform",
              meta = (DefaultToSelf = "InActor"))
    static TArray<FCk_Utils_Actor_SocketTransforms>
    Get_SocketTransform_Exec(
        AActor* InActor,
        FName InSocketName,
        ERelativeTransformSpace InTransformSpace = RTS_World);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Socket Transform (Using Prefix)",
              meta = (DefaultToSelf = "InActor"))
    static TArray<FCk_Utils_Actor_SocketTransforms>
    Get_SocketTransform_UsingPrefix(
        AActor* InActor,
        FName InSocketNamePrefix,
        TEnumAsByte<ESearchCase::Type> InSearchCase = ESearchCase::IgnoreCase,
        ERelativeTransformSpace InTransformSpace = RTS_World);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Get Socket Transform (Using Prefix)",
              meta = (DefaultToSelf = "InActor"))
    static TArray<FCk_Utils_Actor_SocketTransforms>
    Get_SocketTransform_UsingPrefix_Exec(
        AActor* InActor,
        FName InSocketNamePrefix,
        TEnumAsByte<ESearchCase::Type> InSearchCase = ESearchCase::IgnoreCase,
        ERelativeTransformSpace InTransformSpace = RTS_World);

public:
    /**
     * Assigns a new label to this actor. Actor labels are only available in development builds.
     * @param InActor The target actor to change the label on
     * @param InNewActorLabel The new label string to assign to the actor.  If empty, the actor will have a default label.
     * @param InMarkDirty If true the actor's package will be marked dirty for saving.  Otherwise it will not be.  You should pass false for this parameter if dirtying is not allowed (like during loads)
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Actor",
              DisplayName = "[Ck] Request Set Actor Label",
              meta = (DefaultToSelf = "InActor", DevelopmentOnly))
    static void
    Request_SetActorLabel(
        AActor* InActor,
        const FString& InNewActorLabel,
        bool InMarkDirty = true);

public:
    static auto
    Request_SpawnActor(
        const SpawnActorParamsType& InSpawnActorParams,
        const TFunction<void (AActor*)>& InPreFinishSpawningFunc = nullptr) -> AActor*;

    template <typename T_ActorType>
    static auto
    Request_SpawnActor(
        SpawnActorParamsType InSpawnActorParams,
        const TFunction<void (T_ActorType*)>& InPreFinishSpawningFunc = nullptr) -> T_ActorType*;

    static auto
    Request_RemoveActorComponent(
        UActorComponent* InComponentToRemove,
        bool InPromoteChildrenComponents = false) -> void;

public:
    template <typename T_CompType>
    static auto
    Request_AddNewActorComponent(
        const AddNewActorComponent_Params<T_CompType>& InParams,
        TFunction<void (T_CompType*)> InInitializerFunc = nullptr) -> T_CompType*;

private:
    static auto
    DoRequest_BeginDeferredSpawnActor(
        const DeferredSpawnActor_Params& InDeferredSpawnActorParams) -> AActor*;

    static auto
    DoRequest_SpawnActor_Begin(
        const SpawnActorParamsType& InSpawnActorParams) -> AActor*;

    static auto
    DoRequest_SpawnActor_Finish(
        const SpawnActorParamsType& InSpawnActorParams,
        AActor* InNewlySpawnedActor) -> AActor*;

    static auto
    DoRequest_CopyAllActorComponentProperties(
        AActor* InSourceActor,
        AActor* InDestinationActor) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_CompType>
UCk_Utils_Actor_UE::AddNewActorComponent_Params<T_CompType>::
    AddNewActorComponent_Params(
        AActor* InTargetActor)
    : ThisType(InTargetActor, ck::IsValid(InTargetActor) ? InTargetActor->GetRootComponent() : nullptr)
{
}

template <typename T_CompType>
UCk_Utils_Actor_UE::AddNewActorComponent_Params<T_CompType>::
    AddNewActorComponent_Params(
        AActor* InTargetActor,
        USceneComponent* InParent)
    : ThisType(InTargetActor, T_CompType::StaticClass(), InParent)
{
}

template <typename T_CompType>
UCk_Utils_Actor_UE::AddNewActorComponent_Params<T_CompType>::
    AddNewActorComponent_Params(
        AActor* InTargetActor,
        CompClassType InCompClass,
        USceneComponent* InParent)
    : _Owner(InTargetActor)
    , _ParentComp(InParent)
    , _CompClass(InCompClass)
{
}

// --------------------------------------------------------------------------------------------------------------------

template <typename T_ActorType>
auto
    UCk_Utils_Actor_UE::
    Request_SpawnActor(
        SpawnActorParamsType InSpawnActorParams,
        const TFunction<void(T_ActorType*)>& InPreFinishSpawningFunc)
    -> T_ActorType*
{
    InSpawnActorParams._ActorClass = T_ActorType::StaticClass();
    const auto& SpawnedActor = Cast<T_ActorType>(DoRequest_SpawnActor_Begin(InSpawnActorParams));

    if (ck::Is_NOT_Valid(SpawnedActor))
    { return {}; }

    if (InPreFinishSpawningFunc)
    {
        InPreFinishSpawningFunc(SpawnedActor);
    }

    return Cast<T_ActorType>(DoRequest_SpawnActor_Finish(InSpawnActorParams, SpawnedActor));
}

template <typename T_CompType>
auto
    UCk_Utils_Actor_UE::
    Request_AddNewActorComponent(
        const AddNewActorComponent_Params<T_CompType>& InParams,
        TFunction<void(T_CompType*)> InInitializerFunc)
    -> T_CompType*
{
    const auto& Owner = InParams.Get_Owner();

    CK_ENSURE_IF_NOT(ck::IsValid(Owner), TEXT("Cannot add new component of type [{}] because the Owning Actor is INVALID."), InParams.Get_CompClass())
    { return nullptr; }

    if (InParams.Get_IsUnique())
    {
        const auto& ComponentOfSameClass = Owner->GetComponentByClass(InParams.Get_CompClass());

        CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(ComponentOfSameClass),
            TEXT("Trying to add a unique component of type [{}] to Actor [{}] but that component already exists."),
            InParams.Get_CompClass(),
            Owner)
        { return nullptr; }
    }

    auto* Comp = NewObject<T_CompType>(InParams.Get_Owner().Get(), InParams.Get_CompClass());

    if (ck::Is_NOT_Valid(Comp))
    { return Comp; }

    Comp->ComponentTags = InParams.Get_Tags();

    if constexpr (
        std::is_base_of_v<USceneComponent, T_CompType> ||
        std::is_same_v<UActorComponent, T_CompType> ||
        std::is_same_v<UObject, T_CompType>)
    {
        auto* SceneComp  = Cast<USceneComponent>(Comp);

        if (auto* ParentComp = InParams.Get_ParentComp().Get();
            ck::IsValid(SceneComp) && ck::IsValid(ParentComp))
        {
            SceneComp->SetupAttachment(ParentComp, InParams.Get_Socket());
        }
    }

    if (InInitializerFunc)
    {
        InInitializerFunc(Comp);
    }

    InParams.Get_Owner()->FinishAndRegisterComponent(Comp);

    return Comp;
}

// --------------------------------------------------------------------------------------------------------------------
