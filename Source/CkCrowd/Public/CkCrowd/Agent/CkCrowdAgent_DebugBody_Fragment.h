#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPmg/CkPmg_Fragment_Data.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// FCk_Handle_Pmg_DebugShape comes from CkPmg/CkPmg_Fragment_Data.h (above) — it's a stored
// member, not a forward-declarable pointer.

namespace ck
{
    // Per-agent storage for the DrawBody visualization. Stamped by FProcessor_CrowdAgent_DrawBody_Setup
    // once the agent's HasProbe tag lands. Holds typed handles to two PMG child entities (body
    // capsule + forward-facing cone), both SceneNode-parented to the agent's transform so they
    // follow the integrator's per-tick position updates with no per-frame sync code.
    //
    // Lifetime — both child entities are created with PMG `InDuration = -1.0f` so they persist
    // until the agent (their parent) is destroyed; cascade cleanup is automatic.
    //
    // _LastAppliedColor is a small optimization: the Update processor recomputes the state-tinted
    // color each tick but only writes it to the PMG handles when it changes. Avoids spamming
    // Set_Color requests when the agent is in a steady state.
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

    // Tag stamped alongside FFragment_CrowdAgent_DebugBody by the Setup processor. The Setup
    // processor's view excludes this tag so it runs exactly once per agent.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_DebugBody_Setup);
}

// --------------------------------------------------------------------------------------------------------------------
