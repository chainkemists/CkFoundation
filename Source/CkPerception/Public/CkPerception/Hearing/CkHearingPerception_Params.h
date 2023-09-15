#pragma once

#include "CkFormat/CkFormat.h"
#include "CkMacros/CkMacros.h"
#include "CkValidation/CkIsValid.h"

#include <Engine/DataTable.h>

#include "CkHearingPerception_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_HearingPerception_NoiseFiltering_Policy: uint8
{
    PerceiveAllNoises UMETA(DisplayName = "Perceive All Noises"),
    OnlyPerceiveOtherActorNoises UMETA(DisplayName = "Only Perceive Other Actors' Noises"),
    OnlyPerceiveThisActorNoises UMETA(DisplayName = "Only Perceive This Actor's Noises")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_HearingPerception_NoiseFiltering_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPERCEPTION_API FCk_HearingPerception_Noise_DebugInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HearingPerception_Noise_DebugInfo);

public:
    FCk_HearingPerception_Noise_DebugInfo() = default;
    FCk_HearingPerception_Noise_DebugInfo(
        float InLineThickness,
        FColor InDebugLineColor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _LineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FColor _DebugLineColor = FColor::White;

public:
    CK_PROPERTY_GET(_LineThickness);
    CK_PROPERTY_GET(_DebugLineColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPERCEPTION_API FCk_HearingPerception_NoiseReceiver_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HearingPerception_NoiseReceiver_Params);

public:
    FCk_HearingPerception_NoiseReceiver_Params() = default;
    FCk_HearingPerception_NoiseReceiver_Params(
        float InHearingModifier,
        ECk_HearingPerception_NoiseFiltering_Policy InNoiseFilteringPolicy);

private:
    // Modifier applied to noises' emitted travel distance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0))
    float _HearingModifier = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_HearingPerception_NoiseFiltering_Policy _NoiseFilteringPolicy = ECk_HearingPerception_NoiseFiltering_Policy::OnlyPerceiveOtherActorNoises;

public:
    CK_PROPERTY_GET(_HearingModifier);
    CK_PROPERTY_GET(_NoiseFilteringPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPERCEPTION_API FCk_HearingPerception_NoiseEmitter_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HearingPerception_NoiseEmitter_Params);

public:
    FCk_HearingPerception_NoiseEmitter_Params() = default;
    FCk_HearingPerception_NoiseEmitter_Params(
        float InLoudnessModifier,
        bool InShowDebug,
        FCk_HearingPerception_Noise_DebugInfo InDebugParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0))
    float _LoudnessModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _ShowDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_ShowDebug"))
    FCk_HearingPerception_Noise_DebugInfo _DebugParams;

public:
    CK_PROPERTY_GET(_LoudnessModifier);
    CK_PROPERTY_GET(_ShowDebug);
    CK_PROPERTY_GET(_DebugParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_HearingPerception_Listener
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HearingPerception_Listener);

public:
    FCk_HearingPerception_Listener() = default;
    explicit FCk_HearingPerception_Listener(class UCk_HearingPerception_NoiseReceiver_ActorComponent_UE* InNoiseReceiverComp);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(Transient)
    TObjectPtr<class UCk_HearingPerception_NoiseReceiver_ActorComponent_UE> _NoiseReceiverComp;

    UPROPERTY(Transient)
    TObjectPtr<class AActor> _ReceiverOwningActor;

public:
    CK_PROPERTY_GET(_NoiseReceiverComp);
    CK_PROPERTY_GET(_ReceiverOwningActor);
};

CK_DEFINE_CUSTOM_IS_VALID(FCk_HearingPerception_Listener, ck::IsValid_Policy_Default, [=](const FCk_HearingPerception_Listener& InListener)
{
    return ck::IsValid(InListener.Get_NoiseReceiverComp()) && ck::IsValid(InListener.Get_ReceiverOwningActor());
});

auto CKPERCEPTION_API GetTypeHash(const FCk_HearingPerception_Listener& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_HearingPerception_NoiseInfo : public FTableRowBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HearingPerception_NoiseInfo);

public:
    FCk_HearingPerception_NoiseInfo() = default;
    FCk_HearingPerception_NoiseInfo(FGameplayTag InNoiseTag, float InTravelDistance, float InLifetime, USoundBase* InNoiseSound);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _NoiseTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, Units = "cm"))
    float _TravelDistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, Units = "s"))
    float _Lifetime = 0.0f;

    // NOTE/TODO: Sound should NOT be part of the noise system for many reasons. Look into removing it after V1.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<USoundBase> _NoiseSound;

public:
    CK_PROPERTY_GET(_NoiseTag);
    CK_PROPERTY_GET(_TravelDistance);
    CK_PROPERTY_GET(_Lifetime);
    CK_PROPERTY_GET(_NoiseSound);
};

CK_DEFINE_CUSTOM_IS_VALID(FCk_HearingPerception_NoiseInfo, ck::IsValid_Policy_Default, [=](const FCk_HearingPerception_NoiseInfo& InNoiseInfo)
{
    return ck::IsValid(InNoiseInfo.Get_NoiseTag()) && InNoiseInfo.Get_Lifetime() > 0.0f && InNoiseInfo.Get_TravelDistance() > 0.0f/* && ck::IsValid(InNoiseInfo.Get_NoiseSound())*/;
});

auto CKPERCEPTION_API GetTypeHash(const FCk_HearingPerception_NoiseInfo& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_HearingPerception_NoiseEvent : public FTableRowBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HearingPerception_NoiseEvent);

public:
    FCk_HearingPerception_NoiseEvent() = default;
    FCk_HearingPerception_NoiseEvent(FCk_HearingPerception_NoiseInfo InNoiseInfo, FVector InNoiseLocation, AActor* InInstigator);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_HearingPerception_NoiseInfo _NoiseInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _NoiseLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<AActor> _Instigator;

public:
    CK_PROPERTY_GET(_NoiseInfo);
    CK_PROPERTY_GET(_NoiseLocation);
    CK_PROPERTY_GET(_Instigator);
};

CK_DEFINE_CUSTOM_IS_VALID(FCk_HearingPerception_NoiseEvent, ck::IsValid_Policy_Default, [=](const FCk_HearingPerception_NoiseEvent& InNoiseEvent)
{
    return ck::IsValid(InNoiseEvent.Get_Instigator()) && ck::IsValid(InNoiseEvent.Get_NoiseInfo());
});

auto CKPERCEPTION_API GetTypeHash(const FCk_HearingPerception_NoiseEvent& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_HearingPerception_OnEmitNoiseRequested,
    FGameplayTag, InNoiseEventTag,
    FVector, InNoiseLocation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_HearingPerception_OnEmitNoiseRequested_MC,
    FGameplayTag, InNoiseEventTag,
    FVector, InNoiseLocation);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_HearingPerception_OnPerceivedNoiseAdded,
    FCk_HearingPerception_NoiseEvent, InNoiseEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_HearingPerception_OnPerceivedNoiseAdded_MC,
    FCk_HearingPerception_NoiseEvent, InNoiseEvent);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_HearingPerception_OnExistingPerceivedNoiseUpdated,
    FCk_HearingPerception_NoiseEvent, InNoiseEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_HearingPerception_OnExistingPerceivedNoiseUpdated_MC,
    FCk_HearingPerception_NoiseEvent, InNoiseEvent);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_HearingPerception_OnPerceivedNoiseRemoved,
    FCk_HearingPerception_NoiseEvent, InNoiseEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_HearingPerception_OnPerceivedNoiseRemoved_MC,
    FCk_HearingPerception_NoiseEvent, InNoiseEvent);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_HearingPerception_NoiseInfo_DataTable
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HearingPerception_NoiseInfo_DataTable);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(RequiredAssetDataTags="RowStructure=/Script/CkPerception.Ck_HearingPerception_NoiseInfo"))
    TObjectPtr<UDataTable> _NoiseDataTable;
};

// --------------------------------------------------------------------------------------------------------------------
