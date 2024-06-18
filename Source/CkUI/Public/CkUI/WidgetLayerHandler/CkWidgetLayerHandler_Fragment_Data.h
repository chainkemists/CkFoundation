#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkUI/UserWidget/CkUserWidget.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "CkWidgetLayerHandler_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKUI_API FCk_Handle_WidgetLayerHandler : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_WidgetLayerHandler); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_WidgetLayerHandler);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Fragment_WidgetLayerHandler_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_WidgetLayerHandler_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Request_WidgetLayerHandler_PushToLayer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WidgetLayerHandler_PushToLayer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "UI.WidgetLayer"))
    FGameplayTag _Layer;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSoftClassPtr<UCk_UserWidget_UE> _WidgetClass;

public:
    CK_PROPERTY_GET(_Layer);
    CK_PROPERTY_GET(_WidgetClass);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_WidgetLayerHandler_PushToLayer, _Layer, _WidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Request_WidgetLayerHandler_PushToLayer_Instanced
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WidgetLayerHandler_PushToLayer_Instanced);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "UI.WidgetLayer"))
    FGameplayTag _Layer;

    // TODO: If this ever ends up deferred, we will need to make sure the widget doesn't get garbage collected in flight
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_UserWidget_UE> _WidgetInstance;

    public:
    CK_PROPERTY_GET(_Layer);
    CK_PROPERTY_GET(_WidgetInstance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_WidgetLayerHandler_PushToLayer_Instanced, _Layer, _WidgetInstance);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Request_WidgetLayerHandler_PopFromLayer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WidgetLayerHandler_PopFromLayer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "UI.WidgetLayer"))
    FGameplayTag _Layer;
    
public:
    CK_PROPERTY_GET(_Layer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_WidgetLayerHandler_PopFromLayer, _Layer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Request_WidgetLayerHandler_ClearLayer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WidgetLayerHandler_ClearLayer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "UI.WidgetLayer"))
    FGameplayTag _Layer;
    
public:
    CK_PROPERTY_GET(_Layer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_WidgetLayerHandler_ClearLayer, _Layer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Request_WidgetLayerHandler_AddToLayerNamedSlot
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WidgetLayerHandler_AddToLayerNamedSlot);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "UI.WidgetLayer"))
    FGameplayTag _Layer;

    // TODO: If this ever ends up deferred, we will need to make sure the widget doesn't get garbage collected in flight
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_UserWidget_UE> _WidgetInstance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _NamedSlotName;
    
public:
    CK_PROPERTY_GET(_Layer);    
    CK_PROPERTY_GET(_WidgetInstance);
    CK_PROPERTY_GET(_NamedSlotName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_WidgetLayerHandler_AddToLayerNamedSlot, _Layer, _WidgetInstance, _NamedSlotName);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_WidgetLayerHandler_OnPushToLayer,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer,
    TSoftClassPtr<UCk_UserWidget_UE>, WidgetClass);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_WidgetLayerHandler_OnPushToLayer_MC,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer,
    TSoftClassPtr<UCk_UserWidget_UE>, WidgetClass);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_WidgetLayerHandler_OnPushToLayer_Instanced,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer,
    UCk_UserWidget_UE*, WidgetInstance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_WidgetLayerHandler_OnPushToLayer_Instanced_MC,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer,
    UCk_UserWidget_UE*, WidgetInstance);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_WidgetLayerHandler_OnPopFromLayer,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_WidgetLayerHandler_OnPopFromLayer_MC,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_WidgetLayerHandler_OnClearLayer,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_WidgetLayerHandler_OnClearLayer_MC,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_WidgetLayerHandler_OnAddToLayerNamedSlot,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer,
    UCk_UserWidget_UE*, WidgetInstance,
    FName, NamedSlotName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FCk_Delegate_WidgetLayerHandler_OnAddToLayerNamedSlot_MC,
    FCk_Handle_WidgetLayerHandler, InHandle,
    FGameplayTag, Layer,
    UCk_UserWidget_UE*, WidgetInstance,
    FName, NamedSlotName);

// --------------------------------------------------------------------------------------------------------------------
