#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkProvider/Public/CkProvider/CkProvider_Data.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkAnimPlan_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKANIMATION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_AnimPlan_Goal);
CKANIMATION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Label_AnimPlan_Cluster);
CKANIMATION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_AnimPlan_State);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKANIMATION_API FCk_Handle_AnimPlan : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AnimPlan); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AnimPlan);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Fragment_AnimPlan_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AnimPlan_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Goal"))
    FGameplayTag _AnimGoal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Cluster"))
    FGameplayTag _StartingAnimCluster;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.State"))
    FGameplayTag _StartingAnimState;

public:
    CK_PROPERTY_GET(_AnimGoal);
    CK_PROPERTY(_StartingAnimCluster);
    CK_PROPERTY(_StartingAnimState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AnimPlan_ParamsData, _AnimGoal);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Fragment_MultipleAnimPlan_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleAnimPlan_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_AnimGoal"))
    TArray<FCk_Fragment_AnimPlan_ParamsData> _AnimPlanParams;

public:
    CK_PROPERTY_GET(_AnimPlanParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleAnimPlan_ParamsData, _AnimPlanParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimPlan_Goal
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimPlan_Goal);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Goal"))
    FGameplayTag _AnimGoal;

public:
    CK_PROPERTY_GET(_AnimGoal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimPlan_Goal, _AnimGoal);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimPlan_Goal& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_AnimPlan_Goal, [](const FCk_AnimPlan_Goal& InObj)
{
    return ck::Format(TEXT("Goal: {}"), InObj.Get_AnimGoal());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimPlan_Cluster
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimPlan_Cluster);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Goal"))
    FGameplayTag _AnimGoal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Cluster"))
    FGameplayTag _AnimCluster;

public:
    CK_PROPERTY_GET(_AnimGoal);
    CK_PROPERTY_GET(_AnimCluster);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimPlan_Cluster, _AnimGoal, _AnimCluster);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimPlan_Cluster& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_AnimPlan_Cluster, [](const FCk_AnimPlan_Cluster& InObj)
{
    return ck::Format(TEXT("Goal: {} | Cluster: {}"), InObj.Get_AnimGoal(), InObj.Get_AnimCluster());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimPlan_State
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimPlan_State);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Goal"))
    FGameplayTag _AnimGoal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Cluster"))
    FGameplayTag _AnimCluster;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.State"))
    FGameplayTag _AnimState;

public:
    CK_PROPERTY_GET(_AnimGoal);
    CK_PROPERTY_GET(_AnimCluster);
    CK_PROPERTY_GET(_AnimState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimPlan_State, _AnimGoal, _AnimCluster, _AnimState);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimPlan_State& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_AnimPlan_State, [](const FCk_AnimPlan_State& InObj)
{
    return ck::Format(TEXT("Goal: {} | Cluster: {} | State: {}"), InObj.Get_AnimGoal(), InObj.Get_AnimCluster(), InObj.Get_AnimState());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_AnimPlan_UpdateAnimCluster : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AnimPlan_UpdateAnimCluster);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AnimPlan_UpdateAnimCluster);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Cluster"))
    FGameplayTag _NewAnimCluster;

public:
    CK_PROPERTY_GET(_NewAnimCluster);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AnimPlan_UpdateAnimCluster, _NewAnimCluster);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_AnimPlan_UpdateAnimState : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AnimPlan_UpdateAnimState);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AnimPlan_UpdateAnimState);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.Cluster"))
    FGameplayTag _NewAnimCluster;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "AnimPlan.State"))
    FGameplayTag _NewAnimState;

public:
    CK_PROPERTY_GET(_NewAnimCluster);
    CK_PROPERTY_GET(_NewAnimState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AnimPlan_UpdateAnimState, _NewAnimCluster, _NewAnimState);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_AnimPlan_OnPlanChanged,
    FCk_Handle_AnimPlan, InHandle,
    const FCk_AnimPlan_State&, InPreviousPlanState,
    const FCk_AnimPlan_State&, InNewPlanState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_AnimPlan_OnPlanChanged_MC,
    FCk_Handle_AnimPlan, InHandle,
    const FCk_AnimPlan_State&, InPreviousPlanState,
    const FCk_AnimPlan_State&, InNewPlanState);

// --------------------------------------------------------------------------------------------------------------------