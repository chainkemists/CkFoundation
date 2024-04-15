#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "CkAnimState_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKANIMATION_API FCk_Handle_AnimState : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AnimState); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AnimState);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class ECk_AnimStateComponents : uint8
{
    None = 0 UMETA(Hidden),
    Goal = 1 << 0,
    State = 1 << 1,
    Cluster = 1 << 2,
    Overlay = 1 << 3
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AnimStateComponents);
ENABLE_ENUM_BITWISE_OPERATORS(ECk_AnimStateComponents);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimState_Goal
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimState_Goal);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "Animation.Goal"))
    FGameplayTag _AnimGoal;

public:
    CK_PROPERTY_GET(_AnimGoal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimState_Goal, _AnimGoal);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimState_Goal& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER(FCk_AnimState_Goal, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get_AnimGoal());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimState_Cluster
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimState_Cluster);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "Animation.Cluster"))
    FGameplayTag _AnimCluster;

public:
    CK_PROPERTY_GET(_AnimCluster);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimState_Cluster, _AnimCluster);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimState_Cluster& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER(FCk_AnimState_Cluster, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get_AnimCluster());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimState_State
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimState_State);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "Animation.State"))
    FGameplayTag _AnimState;

public:
    CK_PROPERTY_GET(_AnimState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimState_State, _AnimState);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimState_State& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER(FCk_AnimState_State, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get_AnimState());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimState_Overlay
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimState_Overlay);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "Animation.Overlay"))
    FGameplayTag _AnimOverlay;

public:
    CK_PROPERTY_GET(_AnimOverlay);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimState_Overlay, _AnimOverlay);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimState_Overlay& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER(FCk_AnimState_Overlay, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get_AnimOverlay());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_AnimState_SetGoal
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AnimState_SetGoal);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Goal _NewAnimGoal;

public:
    CK_PROPERTY_GET(_NewAnimGoal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AnimState_SetGoal, _NewAnimGoal);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_AnimState_SetCluster
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AnimState_SetCluster);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Cluster _NewAnimCluster;

public:
    CK_PROPERTY_GET(_NewAnimCluster);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AnimState_SetCluster, _NewAnimCluster);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_AnimState_SetState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AnimState_SetState);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_State _NewAnimState;

public:
    CK_PROPERTY_GET(_NewAnimState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AnimState_SetState, _NewAnimState);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_AnimState_SetOverlay
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AnimState_SetOverlay);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Overlay _NewAnimOverlay;

public:
    CK_PROPERTY_GET(_NewAnimOverlay);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AnimState_SetOverlay, _NewAnimOverlay);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Payload_AnimState_OnGoalChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_AnimState_OnGoalChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_AnimState _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Goal _PreviousAnimGoal;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Goal _NewAnimGoal;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_PreviousAnimGoal);
    CK_PROPERTY_GET(_NewAnimGoal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_AnimState_OnGoalChanged, _Handle, _PreviousAnimGoal, _NewAnimGoal);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnGoalChanged,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnGoalChanged&, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnGoalChanged_MC,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnGoalChanged&, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Payload_AnimState_OnClusterChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_AnimState_OnClusterChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_AnimState _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Cluster _PreviousAnimCluster;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Cluster _NewAnimCluster;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_PreviousAnimCluster);
    CK_PROPERTY_GET(_NewAnimCluster);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_AnimState_OnClusterChanged, _Handle, _PreviousAnimCluster, _NewAnimCluster);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnClusterChanged,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnClusterChanged&, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnClusterChanged_MC,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnClusterChanged&, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Payload_AnimState_OnStateChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_AnimState_OnStateChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_AnimState _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_State _PreviousAnimState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_State _NewAnimState;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_PreviousAnimState);
    CK_PROPERTY_GET(_NewAnimState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_AnimState_OnStateChanged, _Handle, _PreviousAnimState, _NewAnimState);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnStateChanged,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnStateChanged&, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnStateChanged_MC,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnStateChanged&, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Payload_AnimState_OnOverlayChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_AnimState_OnOverlayChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_AnimState _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Overlay _PreviousAnimOverlay;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Overlay _NewAnimOverlay;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_PreviousAnimOverlay);
    CK_PROPERTY_GET(_NewAnimOverlay);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_AnimState_OnOverlayChanged, _Handle, _PreviousAnimOverlay, _NewAnimOverlay);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnOverlayChanged,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnOverlayChanged&, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AnimState_OnOverlayChanged_MC,
    FCk_Handle_AnimState, InHandle,
    const FCk_Payload_AnimState_OnOverlayChanged&, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimState_Current
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimState_Current);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Goal _AnimGoal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Cluster _AnimCluster;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_State _AnimState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Overlay _AnimOverlay;

public:
    CK_PROPERTY_GET(_AnimGoal);
    CK_PROPERTY_GET(_AnimCluster);
    CK_PROPERTY_GET(_AnimState);
    CK_PROPERTY_GET(_AnimOverlay);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimState_Current, _AnimGoal, _AnimCluster, _AnimState, _AnimOverlay);
};

auto CKANIMATION_API GetTypeHash(const FCk_AnimState_Current& InObj) -> uint32;

CK_DEFINE_CUSTOM_FORMATTER(FCk_AnimState_Current, [&]()
{
    return ck::Format(TEXT("AnimGoal: [{}], AnimCluster: [{}], AnimState: [{}], AnimOverlay: [{}]"), InObj.Get_AnimGoal(), InObj.Get_AnimCluster(), InObj.Get_AnimState(), InObj.Get_AnimOverlay());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Fragment_AnimState_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AnimState_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnimState_Current _StartingAnimState;

public:
    CK_PROPERTY_GET(_StartingAnimState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AnimState_ParamsData, _StartingAnimState);
};

// --------------------------------------------------------------------------------------------------------------------
