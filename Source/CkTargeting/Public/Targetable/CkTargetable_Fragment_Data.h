#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "CkTargetable_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKTARGETING_API FCk_Handle_Targetable : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Targetable); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Targetable);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Targetable_BasicInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Targetable_BasicInfo);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    auto operator<(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATORS(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Targetable _Targetable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Owner;

public:
    CK_PROPERTY_GET(_Targetable);
    CK_PROPERTY_GET(_Owner);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Targetable_BasicInfo, _Targetable, _Owner);
};

auto CKTARGETING_API GetTypeHash(const FCk_Targetable_BasicInfo& InObj) -> uint32;

CK_DEFINE_CUSTOM_IS_VALID(FCk_Targetable_BasicInfo, ck::IsValid_Policy_Default, [&](const FCk_Targetable_BasicInfo& InTargetable)
{
    return ck::IsValid(InTargetable.Get_Owner(), ck::IsValid_Policy_Default{}) && ck::IsValid(InTargetable.Get_Targetable(), ck::IsValid_Policy_Default{});
});

CK_DEFINE_CUSTOM_IS_VALID(FCk_Targetable_BasicInfo, ck::IsValid_Policy_IncludePendingKill, [&](const FCk_Targetable_BasicInfo& InTargetable)
{
    return ck::IsValid(InTargetable.Get_Owner(), ck::IsValid_Policy_IncludePendingKill{}) && ck::IsValid(InTargetable.Get_Targetable(), ck::IsValid_Policy_IncludePendingKill{});
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Targetable_AttachmentParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Targetable_AttachmentParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _LocalOffset = FVector::ZeroVector;

    // If no name is specified, it'll attach to the root by default
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _BoneName;

public:
    CK_PROPERTY_GET(_LocalOffset);
    CK_PROPERTY_GET(_BoneName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Request_Targetable_EnableDisable
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Targetable_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Targetable_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Targetable_OnEnableDisable,
    FCk_Targetable_BasicInfo, InTargetable,
    ECk_EnableDisable, InEnableDisable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Targetable_OnEnableDisable_MC,
    FCk_Targetable_BasicInfo, InTargetable,
    ECk_EnableDisable, InEnableDisable);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Fragment_Targetable_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Targetable_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _TargetabilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Targetable_AttachmentParams _AttachmentParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_TargetabilityTags);
    CK_PROPERTY_GET(_AttachmentParams);
    CK_PROPERTY_GET(_StartingState);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Fragment_MultipleTargetable_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleTargetable_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_Targetable_ParamsData> _TargetableParams;

public:
    CK_PROPERTY_GET(_TargetableParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleTargetable_ParamsData, _TargetableParams);
};

// --------------------------------------------------------------------------------------------------------------------