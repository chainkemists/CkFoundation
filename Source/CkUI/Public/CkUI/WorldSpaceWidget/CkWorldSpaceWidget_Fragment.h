#pragma once

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment_Data.h"

#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_WorldSpaceWidget_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_WorldSpaceWidget_Params = FCk_Fragment_WorldSpaceWidget_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKUI_API FFragment_WorldSpaceWidget_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_WorldSpaceWidget_Current);

    public:
        friend class FProcessor_WorldSpaceWidget_HandleRequests;
        friend class UCk_Utils_WorldSpaceWidget_UE;

    public:
        FFragment_WorldSpaceWidget_Current() = default;

        explicit
        FFragment_WorldSpaceWidget_Current(
            UUserWidget* InWidget);

    private:
        TStrongObjectPtr<UUserWidget> _WidgetHardRef;
        TWeakObjectPtr<APlayerController> _WidgetOwningPlayer;

    public:
        CK_PROPERTY_GET(_WidgetOwningPlayer);
    };
}

// --------------------------------------------------------------------------------------------------------------------
