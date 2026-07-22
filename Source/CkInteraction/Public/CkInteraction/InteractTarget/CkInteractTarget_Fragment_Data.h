#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"

#include <Engine/BlueprintGeneratedClass.h>
#include <GameplayTagContainer.h>

#include "CkInteractTarget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_InteractTarget_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InteractionTarget_ConcurrentInteractionsPolicy : uint8
{
    SingleInteraction,
    MultipleInteractions
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InteractionTarget_ConcurrentInteractionsPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CanInteractWithResult : uint8
{
    CanInteractWith,
    TargetDisabled,
    TargetNotValid,
    SourceNotValid,
    ChannelMismatch,
    AlreadyExists,
    TargetRejectedSecondInteraction,
    SourceRejectedSecondInteraction,
    CustomValidationFailed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CanInteractWithResult);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINTERACTION_API FCk_Handle_InteractTarget : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InteractTarget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InteractTarget);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_RetVal_ThreeParams(
    bool,
    FCk_Delegate_InteractTarget_CanInteractWith_Native,
    FCk_Handle_InteractTarget, /* InTarget */
    FCk_Handle,                /* InInteractSource */
    FCk_Handle                 /* InInteractInstigator */);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_InteractTarget_CanInteractWith,
    FCk_Handle_InteractTarget, InTarget,
    FCk_Handle, InInteractSource,
    FCk_Handle, InInteractInstigator,
    bool&, OutResult);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_InteractTarget_CanInteractWithReference
{
    GENERATED_BODY()

public:
    FCk_InteractTarget_CanInteractWithReference() = default;

    FCk_InteractTarget_CanInteractWithReference(
        const FName InMemberName,
        const FGuid InMemberGuid,
        UClass* const InMemberClass)
        : _MemberName(InMemberName)
        , _MemberGuid(InMemberGuid)
        , _MemberClass(InMemberClass)
    {}

    auto Get_MemberName() const -> FName
    { return _MemberName; }

    auto Get_MemberGuid() const -> FGuid
    { return _MemberGuid; }

    auto Resolve_MemberClass() const -> UClass*
    { return _MemberClass.LoadSynchronous(); }

private:
    UPROPERTY()
    FName _MemberName;

    UPROPERTY()
    FGuid _MemberGuid;

    // Soft class references preserve the old function picker's load/cook behavior without
    // placing a strong UObject reference into untraced fragment storage.
    UPROPERTY()
    TSoftClassPtr<UObject> _MemberClass;
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_InteractTarget_ParamsData
{
    GENERATED_BODY()

public:
    friend class UCk_Utils_InteractTarget_UE;

public:
    CK_GENERATED_BODY(FCk_Fragment_InteractTarget_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "InteractionChannel"))
    FGameplayTag _InteractionChannel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Interaction_CompletionPolicy _CompletionPolicy = ECk_Interaction_CompletionPolicy::Timed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_CompletionPolicy == ECk_Interaction_CompletionPolicy::Timed", EditConditionHides))
    FCk_Time _InteractionDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InteractionTarget_ConcurrentInteractionsPolicy _ConcurrentInteractionsPolicy = ECk_InteractionTarget_ConcurrentInteractionsPolicy::MultipleInteractions;

    FCk_Delegate_InteractTarget_CanInteractWith_Native _CustomCanInteractWith;

    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Can Interact With",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_InteractTarget_CanInteractWith _CustomCanInteractWithDynamic;

    UPROPERTY()
    FCk_InteractTarget_CanInteractWithReference _CanInteractWithReference;

public:
    CK_PROPERTY_GET(_InteractionChannel);
    CK_PROPERTY(_CompletionPolicy);
    CK_PROPERTY(_InteractionDuration);
    CK_PROPERTY(_ConcurrentInteractionsPolicy);
    CK_PROPERTY(_CustomCanInteractWith);
    CK_PROPERTY(_CustomCanInteractWithDynamic);
    CK_PROPERTY_GET(_CanInteractWithReference);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_InteractTarget_ParamsData, _InteractionChannel);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Blueprint-only authoring wrapper for the function picker. This type is converted to the
 * GC-independent runtime ParamsData before entering any fragment or deferred storage.
 * It intentionally contains FMemberReference and must never itself be retained by an ECS sink.
 */
USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_InteractTarget_SetupData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Target")
    FCk_Fragment_InteractTarget_ParamsData Params;

    UPROPERTY(EditAnywhere, Category = "Interact Target", DisplayName = "Custom Can Interact With",
              meta = (FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInteraction.Ck_Utils_InteractTarget_UE.Prototype_CanInteractWith"))
    FMemberReference CanInteractWithRef;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_MultipleInteractTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleInteractTarget_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_InteractionChannel"))
    TArray<FCk_Fragment_InteractTarget_ParamsData> _InteractTargetParams;

public:
    CK_PROPERTY_GET(_InteractTargetParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleInteractTarget_ParamsData, _InteractTargetParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_MultipleInteractTarget_SetupData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Target")
    TArray<FCk_InteractTarget_SetupData> InteractTargets;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Try_InteractTarget_StartInteraction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_InteractTarget_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Try_InteractTarget_StartInteraction);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _InteractSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _InteractInstigator;

public:
    CK_PROPERTY(_InteractSource)
    CK_PROPERTY(_InteractInstigator)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Try_InteractTarget_StartInteraction, _InteractSource, _InteractInstigator);
};

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractTarget_CancelInteraction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_InteractTarget_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_InteractTarget_CancelInteraction);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractTarget_CancelInteraction);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _InteractSource;

public:
    CK_PROPERTY(_InteractSource)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractTarget_CancelInteraction, _InteractSource);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_InteractTarget_OnNewInteraction,
    FCk_Handle_InteractTarget, InTarget,
    FCk_Handle_Interaction, InInteraction);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_InteractTarget_OnInteractionFinished,
    FCk_Handle_InteractTarget, InTarget,
    FCk_Handle_Interaction, InInteraction,
    ECk_SucceededFailed, SucceededFailed);

// --------------------------------------------------------------------------------------------------------------------
