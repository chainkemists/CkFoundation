#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/SharedValues/CkSharedValues.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkProvider/Public/CkProvider/CkProvider_Data.h"
#include "Targetable/CkTargetable_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkTargeter_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Targeter_TargetingStatus : uint8
{
    // Is continuously targeting
    Targeting,
    NotTargeting
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Targeter_TargetingStatus);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKTARGETING_API FCk_Handle_Targeter : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Targeter); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Targeter);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKTARGETING_API FCk_Targeter_BasicInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Targeter_BasicInfo);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    auto operator<(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATORS(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Targeter _Targeter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Owner;

public:
    CK_PROPERTY_GET(_Targeter);
    CK_PROPERTY_GET(_Owner);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Targeter_BasicInfo, _Targeter, _Owner);
};

auto CKTARGETING_API GetTypeHash(const FCk_Targeter_BasicInfo& InObj) -> uint32;

CK_DEFINE_CUSTOM_IS_VALID(FCk_Targeter_BasicInfo, ck::IsValid_Policy_Default, [&](const FCk_Targeter_BasicInfo& InTargeter)
{
    return ck::IsValid(InTargeter.Get_Owner(), ck::IsValid_Policy_Default{}) && ck::IsValid(InTargeter.Get_Targeter(), ck::IsValid_Policy_Default{});
});

CK_DEFINE_CUSTOM_IS_VALID(FCk_Targeter_BasicInfo, ck::IsValid_Policy_IncludePendingKill, [&](const FCk_Targeter_BasicInfo& InTargeter)
{
    return ck::IsValid(InTargeter.Get_Owner(), ck::IsValid_Policy_IncludePendingKill{}) && ck::IsValid(InTargeter.Get_Targeter(), ck::IsValid_Policy_IncludePendingKill{});
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Targeter_TargetList
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Targeter_TargetList);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Targetable_BasicInfo> _Targets;

public:
    CK_PROPERTY_GET(_Targets);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Targeter_TargetList, _Targets);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKTARGETING_API UCk_Targeter_CustomTargetFilter_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Targeter_CustomTargetFilter_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintnativeEvent,
              BlueprintPure = false,
              Category = "UCk_Targeter_CustomTargetFilter_PDA")
    FCk_Targeter_TargetList
    FilterTargets(
        const FCk_Targeter_BasicInfo& InTargeter,
        const FCk_Targeter_TargetList& InUnfilteredTargets) const;

    UFUNCTION(BlueprintCallable, BlueprintnativeEvent,
              BlueprintPure = false,
              Category = "UCk_Targeter_CustomTargetFilter_PDA")
    FCk_Targeter_TargetList
    SortTargets(
        const FCk_Targeter_BasicInfo& InTargeter,
        const FCk_Targeter_TargetList& InFilteredTargets) const;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Fragment_Targeter_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Targeter_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FGameplayTagQuery _TargetingQuery;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TWeakObjectPtr<UCk_Targeter_CustomTargetFilter_PDA> _CustomTargetFilter;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_TargetingQuery);
    CK_PROPERTY_GET(_CustomTargetFilter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Fragment_MultipleTargeter_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleTargeter_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_Targeter_ParamsData> _TargeterParams;

public:
    CK_PROPERTY_GET(_TargeterParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleTargeter_ParamsData, _TargeterParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKTARGETING_API UCk_Provider_Targeter_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Targeter_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Targeter")
    FCk_Fragment_Targeter_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKTARGETING_API UCk_Provider_Targeter_ParamsData_Literal_PDA : public UCk_Provider_Targeter_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Targeter_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_Targeter_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_Targeter_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Provider_Targeter_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Targeter_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Targeter_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKTARGETING_API UCk_Provider_MultipleTargeter_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleTargeter_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Targeter")
    FCk_Fragment_MultipleTargeter_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKTARGETING_API UCk_Provider_MultipleTargeter_ParamsData_Literal_PDA : public UCk_Provider_MultipleTargeter_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleTargeter_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_MultipleTargeter_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MultipleTargeter_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Provider_MultipleTargeter_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_MultipleTargeter_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_MultipleTargeter_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTARGETING_API FCk_Request_Targeter_QueryTargets
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Targeter_QueryTargets);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Targeter_OnTargetsQueried,
    FCk_Targeter_BasicInfo, InTargeter,
    FCk_Targeter_TargetList, InOrderedTargetList);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Targeter_OnTargetsQueried_MC,
    FCk_Targeter_BasicInfo, InTargeter,
    FCk_Targeter_TargetList, InOrderedTargetList);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Predicate_InTargeter_InTarget_OutResult,
    FCk_Targeter_BasicInfo, InTargeter,
    FCk_Targetable_BasicInfo, InTarget,
    FInstancedStruct, InOptionalPayload,
    FCk_SharedBool, OutResult);

DECLARE_DYNAMIC_DELEGATE_FiveParams(
    FCk_Predicate_InTargeter_In2Targets_OutResult,
    FCk_Targeter_BasicInfo, InTargeter,
    FCk_Targetable_BasicInfo, InTargetA,
    FCk_Targetable_BasicInfo, InTargetB,
    FInstancedStruct, InOptionalPayload,
    FCk_SharedBool, OutResult);

// --------------------------------------------------------------------------------------------------------------------
