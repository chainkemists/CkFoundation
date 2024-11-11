#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkMessaging_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKMESSAGING_API FCk_Handle_Messenger : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Messenger); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Messenger);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Messaging_OnBroadcast,
    FCk_Handle, InHandle,
    FGameplayTag, InMessageName,
    FInstancedStruct, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_Messaging_OnBroadcast_MC,
    FCk_Handle, InHandle,
    FGameplayTag, InMessageName,
    FInstancedStruct, InPayload);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, EditInlineNew)
class CKMESSAGING_API UCk_Message_Definition_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Message_Definition_PDA);

public:
    friend class UCk_K2Node_Message_Broadcast;

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true, Categories = "Message"))
    FGameplayTag _MessageName;

public:
    CK_PROPERTY_GET(_MessageName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Message_Definition
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Message_Definition);

private:
    UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Message_Definition_PDA> _MessageObject;

public:
    CK_PROPERTY(_MessageObject);

};

// --------------------------------------------------------------------------------------------------------------------
