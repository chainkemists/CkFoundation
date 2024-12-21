#pragma once

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKISMRENDERER_API FProcessor_IsmProxy_Static : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Static,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FTag_IsmProxy_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_IsmProxy_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_Dynamic : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Dynamic,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        FTag_IsmProxy_Dynamic,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
