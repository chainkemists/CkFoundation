#include "CkUnrealComponent/CkUnrealComponent_OutlineProcessor.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"
#include "Components/PrimitiveComponent.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_UnrealComponent_Outline_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_UnrealComponent_Outline_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_UnrealComponent_Outline_EndPlay);

namespace ck_unreal_component_outline
{
    auto Remove(const FCk_Handle& InHandle,
                const ck::FFragment_Usf_OutlineApplied_Component& InApplied) -> void
    {
        auto* Component = InApplied.Get_Component().Get();
        if (ck::Is_NOT_Valid(Component)) { return; }
        auto* Subsystem = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(Component);
        if (ck::IsValid(Subsystem)) { Subsystem->Clear_ResolvedOutline(Component, InHandle); }
    }
}

namespace ck
{
    auto FProcessor_UnrealComponent_Outline_Sync::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_UnrealComponent_Current& InCurrent,
        const FFragment_Usf_OutlineResolved& InResolved) -> void
    {
        auto* Component = Cast<UPrimitiveComponent>(InCurrent.Get_Component().Get());
        auto* Preset = InResolved.Get_Preset().Get();
        if (ck::Is_NOT_Valid(Component) || ck::Is_NOT_Valid(Preset)) { return; }

        if (InHandle.Has<FFragment_Usf_OutlineApplied_Component>())
        {
            const auto& Applied = InHandle.Get<FFragment_Usf_OutlineApplied_Component>();
            if (Applied.Get_Component() != Component)
            { ck_unreal_component_outline::Remove(InHandle, Applied); }
        }

        auto* Subsystem = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(Component);
        if (ck::Is_NOT_Valid(Subsystem)) { return; }
        Subsystem->Set_ResolvedOutline(Component, InHandle, InResolved);
        InHandle.AddOrGet<FFragment_Usf_OutlineApplied_Component>() =
            FFragment_Usf_OutlineApplied_Component{Component};
    }

    auto FProcessor_UnrealComponent_Outline_Remove::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Usf_OutlineApplied_Component& InApplied) -> void
    {
        ck_unreal_component_outline::Remove(InHandle, InApplied);
        InHandle.Remove<FFragment_Usf_OutlineApplied_Component>();
    }

    auto FProcessor_UnrealComponent_Outline_EndPlay::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Usf_OutlineApplied_Component& InApplied) -> void
    { ck_unreal_component_outline::Remove(InHandle, InApplied); }
}
