#include "CkSnapshot_Module.h"

#include "CkSnapshot/Subsystem/CkSnapshot_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Engine/GameInstance.h>
#include <Engine/World.h>

#define LOCTEXT_NAMESPACE "FCkSnapshotModule"

// --------------------------------------------------------------------------------------------------------------------

void FCkSnapshotModule::StartupModule()
{
    // A world that comes up mid-load has to be held before ANYTHING in it ticks, and the loader cannot do that
    // itself: its own next opportunity is a full world tick later, which is the frame every level actor's
    // BeginPlay and every entity-script construction runs in. CkEcs asks this question at OnWorldBeginPlay; the
    // answer — and the freeze that goes with it — is CkSnapshot's, registered here so CkEcs never learns this
    // module exists.
    UCk_EcsWorld_Subsystem_UE::Set_LoadHoldSeedProvider(
    [](UWorld& InWorld) -> bool
    {
        auto* GameInstance = InWorld.GetGameInstance();
        if (ck::Is_NOT_Valid(GameInstance))
        { return false; }

        auto* Snapshot = GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>();
        if (ck::Is_NOT_Valid(Snapshot))
        { return false; }

        return Snapshot->DoGet_ShouldHoldWorldAtBoot(InWorld);
    });
}

void FCkSnapshotModule::ShutdownModule()
{
    UCk_EcsWorld_Subsystem_UE::Clear_LoadHoldSeedProvider();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkSnapshotModule, CkSnapshot)
