#pragma once

#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

class USkeletalMeshComponent;

namespace ck::iskm
{
    // Complete overwrite of every component-level render setting owned by RendererData.  This is deliberately
    // not a delta API: pooled SKMCs must not retain a previous borrower's disabled shadow/lighting state.
    CKISKMRENDERER_API auto
    MakeRuntimeProfileTuners(
        const UCk_IskmRenderer_Data& InData) -> FCk_IskmRenderer_RuntimeProfileTuners;

    CKISKMRENDERER_API auto
    Get_AreRuntimeProfileTunersValid(
        const FCk_IskmRenderer_RuntimeProfileTuners& InTuners) -> bool;

    CKISKMRENDERER_API auto
    Apply_RenderProfile(
        USkeletalMeshComponent& InComponent,
        const FCk_IskmRenderer_RuntimeProfileTuners& InTuners) -> void;

    CKISKMRENDERER_API auto
    Apply_RenderProfile(
        USkeletalMeshComponent& InComponent,
        const UCk_IskmRenderer_Data& InData) -> void;
}
