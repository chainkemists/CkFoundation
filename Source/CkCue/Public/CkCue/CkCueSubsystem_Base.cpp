#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"
#include "CkCue/Settings/CkCue_Settings.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

#if WITH_EDITOR
#include <Editor.h>
#include "Engine/Blueprint.h"
#endif

/*─────────────────────────────────────────────────────────────────────────────┐
│                           CONSOLE COMMANDS                                   │
└─────────────────────────────────────────────────────────────────────────────*/

static FAutoConsoleCommand ConsoleCommand_PrintCues(
    TEXT("ck.cue.print"),
    TEXT("Print all discovered cues from all cue subsystems"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        int32 SubsystemCount = 0;
        int32 TotalCues = 0;

        for (TObjectIterator<UCk_CueSubsystem_Base_UE> It; It; ++It)
        {
            auto CueSubsystem = *It;
            if (ck::Is_NOT_Valid(CueSubsystem))
            { continue; }

            SubsystemCount++;
            const auto& DiscoveredCues = CueSubsystem->Get_DiscoveredCues();

            ck::cue::Log(TEXT("=== SUBSYSTEM: [{}] ==="), CueSubsystem->GetClass()->GetName());
            ck::cue::Log(TEXT("Discovered cues: [{}]"), DiscoveredCues.Num());

            for (const auto& [CueName, CueClass] : DiscoveredCues)
            {
                ck::cue::Log(TEXT("  [{}] -> [{}]"), CueName, CueClass->GetName());
                TotalCues++;
            }
        }

        ck::cue::Log(TEXT("=== SUMMARY ==="));
        ck::cue::Log(TEXT("Total subsystems: [{}], Total cues: [{}]"), SubsystemCount, TotalCues);

        if (SubsystemCount == 0)
        {
            ck::cue::Warning(TEXT("No CueSubsystems found!"));
        }
    })
);

static FAutoConsoleCommand ConsoleCommand_RediscoverCues(
    TEXT("ck.cue.rediscover"),
    TEXT("Force re-discovery of all cues in all cue subsystems"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        int32 SubsystemCount = 0;

        for (TObjectIterator<UCk_CueSubsystem_Base_UE> It; It; ++It)
        {
            auto CueSubsystem = *It;
            if (ck::Is_NOT_Valid(CueSubsystem))
            { continue; }

            SubsystemCount++;
            ck::cue::Log(TEXT("Re-discovering cues for subsystem: [{}]"), CueSubsystem->GetClass()->GetName());
            CueSubsystem->Request_PopulateAllCues();
            ck::cue::Log(TEXT("  Found [{}] cues"), CueSubsystem->Get_DiscoveredCues().Num());
        }

        if (SubsystemCount == 0)
        {
            ck::cue::Warning(TEXT("No CueSubsystems found!"));
        }
        else
        {
            ck::cue::Log(TEXT("Re-discovery complete for [{}] subsystems"), SubsystemCount);
        }
    })
);

/*─────────────────────────────────────────────────────────────────────────────┐
│                             INTERNAL HELPERS                                  │
└─────────────────────────────────────────────────────────────────────────────*/

namespace ck_cue_subsystem_base
{
    auto
        ExecuteCueEntityScript(
            FCk_Handle InOwnerEntity,
            const FGameplayTag& InCueName,
            TSubclassOf<UCk_CueBase_EntityScript> InCueClass,
            const FInstancedStruct& InSpawnParams)
        -> FCk_Handle_PendingEntityScript
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InCueClass),
            TEXT("CueClass was INVALID when trying to execute Cue [{}]"), InCueName)
        { return {}; }

        auto CueDefaultObject = InCueClass->GetDefaultObject<UCk_CueBase_EntityScript>();

        if (ck::Is_NOT_Valid(InOwnerEntity))
        {
            if (ck::IsValid(CueDefaultObject) &&
                CueDefaultObject->Get_OwnerValidationPolicy() == ECk_Cue_OwnerValidationPolicy::RequireValid)
            {
                CK_ENSURE_IF_NOT(false,
                    TEXT("OwnerEntity is invalid when trying to execute Cue [{}] (OwnerValidationPolicy = RequireValid)"), InCueName)
                { return {}; }
            }

            ck::cue::Verbose(TEXT("Skipping cue [{}] - OwnerEntity is invalid (OwnerValidationPolicy = SkipIfInvalid)"), InCueName);
            return {};
        }

        if (ck::IsValid(CueDefaultObject) &&
            CueDefaultObject->Get_ConcurrencyPolicy() == ECk_Cue_ConcurrencyPolicy::RestartExisting)
        {
            if (auto ExistingCue = ck::ActiveCues_Utils::Get_ValidEntry_ByTag(InOwnerEntity, InCueName);
                ck::IsValid(ExistingCue))
            {
                if (const auto CueScript = Cast<UCk_CueBase_EntityScript>(ExistingCue.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get());
                    ck::IsValid(CueScript))
                {
                    ck::cue::Verbose(TEXT("Restarting existing cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
                    UCk_Utils_EntityScript_UE::TryInjectEntityScriptSpawnParams(CueScript, InSpawnParams);
                    CueScript->Restart();
                    return FCk_Handle_PendingEntityScript{ExistingCue};
                }
            }
        }

        ck::cue::Verbose(TEXT("Executing cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
        auto PendingEntityScript = UCk_Utils_EntityScript_UE::Request_SpawnEntity(InOwnerEntity, InCueClass, InSpawnParams);

        return PendingEntityScript;
    }

    auto
        Get_CueSubsystemFromClass(
            TSubclassOf<UCk_CueSubsystem_Base_UE> InCueSubsystemClass)
        -> UCk_CueSubsystem_Base_UE*
    {
        CK_ENSURE_IF_NOT(ck::IsValid(GEngine),
            TEXT("GEngine is invalid when trying to get CueSubsystem"))
        { return {}; }

        CK_ENSURE_IF_NOT(ck::IsValid(InCueSubsystemClass),
            TEXT("CueSubsystemClass is invalid"))
        { return {}; }

        return Cast<UCk_CueSubsystem_Base_UE>(GEngine->GetEngineSubsystemBase(InCueSubsystemClass));
    }
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                         GENERIC CUE IMPLEMENTATIONS                          │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    UCk_GenericCueExecutor_Subsystem_UE::
    Get_GroupTag() const
    -> FGameplayTag
{
    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.Cue.Generic"));
}

auto
    UCk_GenericCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_GenericCueSubsystem_UE::StaticClass();
}

auto
    UCk_GenericCueExecutor_Subsystem_UE::
    Get_DedicatedServerPolicy() const
    -> ECk_Cue_DedicatedServerPolicy
{
    return ECk_Cue_DedicatedServerPolicy::GameplayRelevant;
}

auto
    UCk_GenericCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_GenericCue_EntityScript::StaticClass();
}
