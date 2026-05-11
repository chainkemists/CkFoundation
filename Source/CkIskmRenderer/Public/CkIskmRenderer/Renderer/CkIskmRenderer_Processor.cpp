#include "CkIskmRenderer/Renderer/CkIskmRenderer_Processor.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/CkIskmSubsystem.h"
#include "CkIskmRenderer/CkIskmRenderer_Log.h"

namespace ck
{
    auto
        FProcessor_IskmRenderer_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmRenderer_Params& InParams,
            FFragment_IskmRenderer_Current& InCurrent) const -> void
    {
        auto* RendererData = InParams.Get_RendererData().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(RendererData),
            TEXT("IskmRenderer Setup: RendererData became invalid for [{}]"), InHandle)
        { return; }

        auto* AnimCollection = RendererData->Get_AnimCollection().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(AnimCollection),
            TEXT("IskmRenderer Setup: RendererData [{}] has invalid AnimCollection (entity [{}])"), RendererData, InHandle)
        { return; }

        auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("IskmRenderer Setup: no world for entity [{}]"), InHandle)
        { return; }

        auto* RendererActor = UCk_Utils_IskmRenderer_Subsystem_UE::GetOrCreate_RendererActor(World, RendererData);
        CK_ENSURE_IF_NOT(ck::IsValid(RendererActor),
            TEXT("IskmRenderer Setup: subsystem could not create renderer actor for [{}]"), InHandle)
        { return; }

        InCurrent._RendererActor = RendererActor;
        InHandle.Remove<FTag_IskmRenderer_NeedsSetup>();
        // Phase N: forward-compat cleanup. Plan-1 never sets _PendingAsyncLoad
        // (Add takes a hard pointer), so the tag is expected-absent here —
        // use Try_Remove to avoid the "tag does not exist" ensure. A future
        // caller that marks the entity pending-async before resolving the
        // asset will benefit from this cleanup once the actor is wired up.
        InHandle.Try_Remove<FTag_IskmRenderer_PendingAsyncLoad>();
    }
}

CK_REGISTER_PROCESSOR(ck::FProcessor_IskmRenderer_Setup);
