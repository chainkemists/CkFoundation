#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkRelationship/Team/CkTeam_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRELATIONSHIP_API FProcessor_TeamAssignedSignal_Setup : public TProcessor<
        FProcessor_TeamAssignedSignal_Setup,
        FTag_TeamListener_Setup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_TeamListener_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRELATIONSHIP_API FProcessor_OnTeamAssigned_Setup : public ck_exp::TProcessor<
        FProcessor_OnTeamAssigned_Setup,
        FCk_Handle_Team,
        FFragment_TeamInfo,
        FTag_OnTeamAssigned_Setup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_OnTeamAssigned_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TeamInfo& InTeamInfo) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
