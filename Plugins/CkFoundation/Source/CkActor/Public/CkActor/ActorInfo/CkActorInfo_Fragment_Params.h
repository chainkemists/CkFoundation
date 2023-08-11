#pragma once

#include "Actor/CkActor.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkMacros/CkMacros.h"

#include "Component/CkActorComponent.h"

#include "CkActorInfo_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_ActorInfo_BasicDetails
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ActorInfo_BasicDetails);

public:
    FCk_ActorInfo_BasicDetails() = default;
    FCk_ActorInfo_BasicDetails(AActor* InActor, FCk_Handle InHandle);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor> _Actor;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Handle;

public:
    CK_PROPERTY_GET(_Actor)
    CK_PROPERTY_GET(_Handle)
};

auto CKACTOR_API GetTypeHash(const FCk_ActorInfo_BasicDetails& InBasicDetails) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKACTOR_API FCk_Fragment_ActorInfo_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ActorInfo_ParamsData);

public:
    FCk_Fragment_ActorInfo_ParamsData() = default;
    FCk_Fragment_ActorInfo_ParamsData(
        TSubclassOf<AActor> InActorClass,
        FTransform InActorSpawnTransform,
        TObjectPtr<AActor> InOwner,
        ECk_Actor_NetworkingType InNetworkingType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<AActor> _ActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _ActorSpawnTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<AActor> _Owner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Actor_NetworkingType _NetworkingType = ECk_Actor_NetworkingType::Local;

public:
    CK_PROPERTY_GET(_ActorClass);
    CK_PROPERTY_GET(_ActorSpawnTransform);
    CK_PROPERTY_GET(_Owner);
    CK_PROPERTY_GET(_NetworkingType);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKACTOR_API UCk_ActorInfo_ActorComponent_UE
    : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorInfo_ActorComponent_UE);

public:
    friend class UCk_Utils_ActorInfo_UE;

private:
    FCk_Handle _EntityHandle;

public:
    CK_PROPERTY_GET(_EntityHandle);
};

// --------------------------------------------------------------------------------------------------------------------
// IsValid and Formatters

CK_DEFINE_CUSTOM_IS_VALID(FCk_ActorInfo_BasicDetails, ck::IsValid_Policy_Default,
[=](const FCk_ActorInfo_BasicDetails& InBasicDetails)
{
    return ck::IsValid(InBasicDetails.Get_Actor()) && ck::IsValid(InBasicDetails.Get_Handle());
});

CK_DEFINE_CUSTOM_FORMATTER(FCk_ActorInfo_BasicDetails, [&]()
{
    return ck::Format
    (
        TEXT("Actor: [{}], Handle: [{}]"),
        InObj.Get_Actor(),
        InObj.Get_Handle()
    );
});

// --------------------------------------------------------------------------------------------------------------------
