#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPmg/CkPmg_Fragment_Data_DebugShapes.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// FCk_Handle_Pmg_DebugShape is a stored member, so the CkPmg header above cannot be a forward decl.

namespace ck
{
    // Handles to the two PMG child entities (body capsule + forward cone) backing the DrawBody
    // visualization. Both are created with PMG `InDuration = -1.0f` and SceneNode-parented to the
    // agent, so they follow it and cascade-destroy with it.
    struct CKCROWD_API FFragment_CrowdAgent_DebugBody
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_DebugBody);
        friend class FProcessor_CrowdAgent_DrawBody_Setup;
        friend class FProcessor_CrowdAgent_DrawBody_Update;

    private:
        FCk_Handle_Pmg_DebugShape _CapsuleHandle{};
        FCk_Handle_Pmg_DebugShape _ConeHandle{};

        FLinearColor _LastAppliedColor   = FLinearColor::White;
        bool         _LastAppliedVisible = false;

    public:
        CK_PROPERTY_GET(_CapsuleHandle);
        CK_PROPERTY_GET(_ConeHandle);
        CK_PROPERTY_GET(_LastAppliedColor);
        CK_PROPERTY_GET(_LastAppliedVisible);
    };

    // Excluded by the Setup processor's own view, which is what makes it run exactly once per agent.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_DebugBody_Setup);
}

// --------------------------------------------------------------------------------------------------------------------
