#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The console-driven gates GroundNav reads.
//
// Most of them make a GroundNav answer WRONG on purpose, so a fixture can prove that the condition it
// settles on is load-bearing: a test that waits for something and passes either way has not pinned
// anything, and the only way to tell the two apart is to run it once with the condition removed and
// watch it fail. The rest switch off work that only a viewer consumes.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::debug
{
    /**
     * Whether ck.GroundNav.Debug.MarkupLiveGate has been turned off, in which case GroundNav's
     * `_IsMarkupLive` must answer TRUE without asking the field.
     *
     * Consulted by the provider adapter and by nothing else. Default 1, so the shipping answer is the
     * field's own and no run behaves differently unless it was asked to.
     */
    CKGROUNDNAV_API auto
    Get_IsMarkupLiveGateBypassed() -> bool;

    /**
     * Whether ck.GroundNav.Debug.RepathOnRebuild has been turned off, in which case the path
     * invalidator must raise FTag_GroundNavPath_RepathRequired for nobody, however much ground moved.
     *
     * Consulted by FProcessor_GroundNavPath_InvalidateOnRebuilt; the debug reports read it only to say whether an answer is being bypassed. Default 1, so the
     * shipping answer is the corridor's own and no run behaves differently unless it was asked to.
     */
    CKGROUNDNAV_API auto
    Get_IsRepathOnRebuildBypassed() -> bool;

    /** Whether ck.GroundNav.Debug.DrawMarkup is on, so the debug views collect and outline the area
     *  markup the world's volumes hold. */
    CKGROUNDNAV_API auto
    Get_IsMarkupDrawEnabled() -> bool;

    /**
     * Whether ck.GroundNav.PathDiagnostics is on, so the per-agent diagnostics pass runs.
     *
     * Nothing planning, walking or replanning reads what that pass writes - only a viewer does - so a
     * world that has no viewer open can stop paying for it. Turning it off leaves whatever was already
     * stamped exactly where it stands: the pass composes and writes, and never removes.
     */
    CKGROUNDNAV_API auto
    Get_IsPathDiagnosticsEnabled() -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
