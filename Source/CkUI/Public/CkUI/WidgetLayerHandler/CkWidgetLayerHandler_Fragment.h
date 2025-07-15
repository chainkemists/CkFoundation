#pragma once

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkWidgetLayerHandler_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_WidgetLayerHandler_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKUI_API FFragment_WidgetLayerHandler_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_WidgetLayerHandler_Params);

    public:
        using ParamsType = FCk_Fragment_WidgetLayerHandler_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_WidgetLayerHandler_Params, _Params);
    };

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUI_API,
        WidgetLayerHandler_OnPushToLayer,
        FCk_Delegate_WidgetLayerHandler_OnPushToLayer_MC,
        FCk_Handle_WidgetLayerHandler,
        FGameplayTag,
        TSoftClassPtr<UCk_UserWidget_UE>);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUI_API,
        WidgetLayerHandler_OnPushToLayer_Instanced,
        FCk_Delegate_WidgetLayerHandler_OnPushToLayer_Instanced_MC,
        FCk_Handle_WidgetLayerHandler,
        FGameplayTag,
        TWeakObjectPtr<UCk_UserWidget_UE>);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUI_API,
        WidgetLayerHandler_OnPopFromLayer,
        FCk_Delegate_WidgetLayerHandler_OnPopFromLayer_MC,
        FCk_Handle_WidgetLayerHandler,
        FGameplayTag);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUI_API,
        WidgetLayerHandler_OnClearLayer,
        FCk_Delegate_WidgetLayerHandler_OnClearLayer_MC,
        FCk_Handle_WidgetLayerHandler,
        FGameplayTag);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUI_API,
        WidgetLayerHandler_OnAddWidgetToLayerNamedSlot,
        FCk_Delegate_WidgetLayerHandler_OnAddWidgetToLayerNamedSlot_MC,
        FCk_Handle_WidgetLayerHandler,
        FGameplayTag,
        TSoftClassPtr<UCk_UserWidget_UE>,
        FName);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUI_API,
        WidgetLayerHandler_OnAddWidgetInstanceToLayerNamedSlot,
        FCk_Delegate_WidgetLayerHandler_OnAddWidgetInstanceToLayerNamedSlot_MC,
        FCk_Handle_WidgetLayerHandler,
        FGameplayTag,
        TWeakObjectPtr<UCk_UserWidget_UE>,
        FName);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUI_API,
        WidgetLayerHandler_OnRemoveWidgetFromLayerNamedSlot,
        FCk_Delegate_WidgetLayerHandler_OnRemoveWidgetFromLayerNamedSlot_MC,
        FCk_Handle_WidgetLayerHandler,
        FGameplayTag,
        FName);
}

// --------------------------------------------------------------------------------------------------------------------