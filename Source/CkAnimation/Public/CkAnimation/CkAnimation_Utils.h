#pragma once

#include <PlayMontageCallbackProxy.h>

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Time/CkTime.h"

#include "CkAnimation_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Animation_MontageSection_LengthInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Animation_MontageSection_LengthInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _StartTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _EndTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _Length;

public:
    CK_PROPERTY(_StartTime);
    CK_PROPERTY(_EndTime);
    CK_PROPERTY(_Length);

public:
    FCk_Animation_MontageSection_LengthInfo() = default;
    FCk_Animation_MontageSection_LengthInfo(
        FCk_Time&& _StartTime,
        FCk_Time&& _EndTime)
        : _StartTime(std::move(_StartTime))
        , _EndTime(std::move(_EndTime))
        , _Length(this->_EndTime - this->_StartTime)
    {};
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Animation_MontageNotify_TimeInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Animation_MontageNotify_TimeInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _StartTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _EndTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _Length;

public:
    CK_PROPERTY(_StartTime);
    CK_PROPERTY(_EndTime);
    CK_PROPERTY(_Length);

public:
    FCk_Animation_MontageNotify_TimeInfo() = default;
    FCk_Animation_MontageNotify_TimeInfo(
        FCk_Time&& _StartTime,
        FCk_Time&& _EndTime)
        : _StartTime(std::move(_StartTime))
        , _EndTime(std::move(_EndTime))
        , _Length(this->_EndTime - this->_StartTime)
    {};
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "UAnimMontage"))
class CKANIMATION_API UCk_Utils_Animation_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Animation_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Montage Section Length (By Index)",
              Category = "Ck|Utils|Animation")
    static FCk_Animation_MontageSection_LengthInfo
    Get_MontageSectionLength_ByIndex(
        UAnimMontage* InAnimMontage,
        int32 InSectionIndex);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Montage Section Length (By Name)",
              Category = "Ck|Utils|Animation")
    static FCk_Animation_MontageSection_LengthInfo
    Get_MontageSectionLength_ByName(
        UAnimMontage* InAnimMontage,
        FName InSectionName);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Montage Section Index (From Position)",
              Category = "Ck|Utils|Animation")
    static int32
    Get_MontageSectionIndex_FromPosition(
        UAnimMontage* InAnimMontage,
        float InPosition);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Valid Montage Section Index",
              Category = "Ck|Utils|Animation")
    static bool
    Get_IsValidMontageSectionIndex(
        UAnimMontage* InAnimMontage,
        int32 InSectionIndex);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Does Montage Match Mesh Skeleton",
              Category = "Ck|Utils|Animation")
    static bool
    DoesMontageMatchMeshSkeleton(
        UAnimMontage* InMontage,
        USkeletalMeshComponent* InSkeletalMeshComponent);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck] Try Get Montage Notify Time",
              Category = "Ck|Utils|Animation",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Animation_MontageNotify_TimeInfo
    TryGet_MontageNotifyTime(
        UAnimMontage* InAnimMontage,
        FName InNotifyName,
        ECk_SucceededFailed& OutResult);

	// This exists to expose play montage to Angelscript which doesn't have access to the play montage task
	UFUNCTION(BlueprintCallable,
              DisplayName="[Ck] Request Play Montage",
              Category = "Ck|Utils|Animation")
    static UPlayMontageCallbackProxy*
    Request_PlayMontage(
		USkeletalMeshComponent* InSkeletalMeshComponent,
		UAnimMontage* MontageToPlay,
		float PlayRate = 1.f,
		float StartingPosition = 0.f,
		FName StartingSection = NAME_None,
		bool ShouldStopAllMontages = true);
};

// --------------------------------------------------------------------------------------------------------------------
