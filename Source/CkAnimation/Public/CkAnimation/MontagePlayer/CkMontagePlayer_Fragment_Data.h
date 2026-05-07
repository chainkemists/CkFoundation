#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <Animation/AnimMontage.h>
#include <Components/SkeletalMeshComponent.h>

#include "CkMontagePlayer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * MontagePlayer signal contract:
 *
 *  - `MontagePlayer_OnStarted` and `MontagePlayer_OnFinished` are AUTHORITATIVE on the originating
 *    machine. Clients only see the *terminal* state of each replication interval. If the server
 *    runs `Play(A) -> Stop` in a single tick, clients see `OnFinished` for whatever montage existed
 *    before this tick and may never see `OnStarted(A)`. Use a different mechanism (e.g., a dedicated
 *    cue) for tightly-paired Started/Finished events on combo-window mechanics.
 */

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKANIMATION_API FCk_Handle_MontagePlayer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_MontagePlayer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_MontagePlayer);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MontagePlayer_StateKind : uint8
{
    Play,
    Stop,
    Pause,
    Resume,
    JumpToSection,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MontagePlayer_StateKind);

// --------------------------------------------------------------------------------------------------------------------

// Reason a MontagePlayer terminated. Pre-flight failures (Failed_*) mean the montage never began
// playback; runtime termination (Completed/Interrupted) means it did. OnFinished broadcasts ALL
// of these — listeners that only care about successful runs should switch on Completed.
UENUM(BlueprintType)
enum class ECk_MontagePlayer_FinishReason : uint8
{
    // Runtime termination
    Completed,
    Interrupted,

    // Pre-flight failures (montage never played)
    Failed_InvalidMontage,
    Failed_InvalidMeshComponent,
    Failed_InvalidPlayRate,
    Failed_MissingAnimInstance,
    Failed_SkeletonMismatch,
    Failed_MeshTickDisabled,
    Failed_MeshComponentCannotTickOnServer,

    // Runtime failure (post-preflight, e.g., slot mismatch on Montage_Play)
    Failed_SlotMismatch,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MontagePlayer_FinishReason);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Fragment_MontagePlayer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MontagePlayer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<USkeletalMeshComponent> _SkeletalMeshComponent;

public:
    CK_PROPERTY_GET(_SkeletalMeshComponent);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MontagePlayer_ParamsData, _SkeletalMeshComponent);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_MontagePlayer_State
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_MontagePlayer_State);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY()
    TObjectPtr<UAnimMontage> _Montage;

    UPROPERTY()
    FName _SectionName = NAME_None;

    UPROPERTY()
    FCk_Time _StartPosition;

    UPROPERTY()
    float _PlayRate = 1.0f;

    UPROPERTY()
    FCk_Time _BlendInTime{0.25};

    UPROPERTY()
    FCk_Time _BlendOutTime{0.25};

    UPROPERTY()
    FCk_Time _ServerStartTime;

    UPROPERTY()
    int32 _PlayInstanceId = 0;

    UPROPERTY()
    ECk_MontagePlayer_StateKind _Kind = ECk_MontagePlayer_StateKind::Play;

public:
    CK_PROPERTY(_Montage);
    CK_PROPERTY(_SectionName);
    CK_PROPERTY(_StartPosition);
    CK_PROPERTY(_PlayRate);
    CK_PROPERTY(_BlendInTime);
    CK_PROPERTY(_BlendOutTime);
    CK_PROPERTY(_ServerStartTime);
    CK_PROPERTY(_PlayInstanceId);
    CK_PROPERTY(_Kind);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_MontagePlayer_State, _Montage);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_MontagePlayer_State, [](const FCk_MontagePlayer_State& InObj)
{
    return ck::Format(TEXT("Montage: {} | Kind: {} | Id: {}"),
        InObj.Get_Montage(), InObj.Get_Kind(), InObj.Get_PlayInstanceId());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_MontagePlayer_Play : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_MontagePlayer_Play);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_MontagePlayer_Play);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimMontage> _Montage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FName _SectionName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _StartPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _PlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _BlendInTime{0.25};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _BlendOutTime{0.25};

    int32 _AuthoritativePlayInstanceId = INDEX_NONE;
    FCk_Time _AuthoritativeServerStartTime;
    bool _FromReplication = false;

public:
    CK_PROPERTY(_Montage);
    CK_PROPERTY(_SectionName);
    CK_PROPERTY(_StartPosition);
    CK_PROPERTY(_PlayRate);
    CK_PROPERTY(_BlendInTime);
    CK_PROPERTY(_BlendOutTime);
    CK_PROPERTY(_AuthoritativePlayInstanceId);
    CK_PROPERTY(_AuthoritativeServerStartTime);
    CK_PROPERTY(_FromReplication);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_MontagePlayer_Play, _Montage);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_MontagePlayer_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_MontagePlayer_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_MontagePlayer_Stop);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _BlendOutTime{0.25};

    int32 _AuthoritativePlayInstanceId = INDEX_NONE;
    bool _FromReplication = false;

public:
    CK_PROPERTY(_BlendOutTime);
    CK_PROPERTY(_AuthoritativePlayInstanceId);
    CK_PROPERTY(_FromReplication);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_MontagePlayer_Stop, _BlendOutTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_MontagePlayer_Pause : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_MontagePlayer_Pause);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_MontagePlayer_Pause);

private:
    int32 _AuthoritativePlayInstanceId = INDEX_NONE;
    bool _FromReplication = false;

public:
    CK_PROPERTY(_AuthoritativePlayInstanceId);
    CK_PROPERTY(_FromReplication);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_MontagePlayer_Resume : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_MontagePlayer_Resume);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_MontagePlayer_Resume);

private:
    int32 _AuthoritativePlayInstanceId = INDEX_NONE;
    bool _FromReplication = false;

public:
    CK_PROPERTY(_AuthoritativePlayInstanceId);
    CK_PROPERTY(_FromReplication);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Request_MontagePlayer_JumpToSection : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_MontagePlayer_JumpToSection);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_MontagePlayer_JumpToSection);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FName _SectionName = NAME_None;

    int32 _AuthoritativePlayInstanceId = INDEX_NONE;
    bool _FromReplication = false;

public:
    CK_PROPERTY(_SectionName);
    CK_PROPERTY(_AuthoritativePlayInstanceId);
    CK_PROPERTY(_FromReplication);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_MontagePlayer_JumpToSection, _SectionName);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_MontagePlayer_OnStarted,
    FCk_Handle_MontagePlayer, InHandle,
    const FCk_MontagePlayer_State&, InState);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_MontagePlayer_OnFinished,
    FCk_Handle_MontagePlayer, InHandle,
    const FCk_MontagePlayer_State&, InState,
    ECk_MontagePlayer_FinishReason, InReason);

// --------------------------------------------------------------------------------------------------------------------
