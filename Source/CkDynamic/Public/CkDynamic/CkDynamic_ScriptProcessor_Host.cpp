#include "CkDynamic_ScriptProcessor_Host.h"

#include "CkDynamic_Utils.h"
#include "CkDynamic_ScriptQueryProcessor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Processor/CkProcessor_Script.h"
#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"
#include "CkEcs/Scheduler/CkProcessorDescriptor.h"
#include "CkEcs/Scheduler/CkProcessorRegistry.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcs/CkEcsLog.h"

#include <Engine/Engine.h>
#include <Engine/World.h>
#include <UObject/Class.h>
#include <UObject/UObjectHash.h>
#include <UObject/UObjectIterator.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace
    {
        // Delegate handle returned by UCk_EcsWorld_Subsystem_UE::Get_OnPreBuildProcessorGraph().Add — retained so
        // ShutdownModule can remove the binding. Static because the registration happens at module scope.
        FDelegateHandle GOnPreBuildHandle;

        // Delegate handle for FAngelscriptCodeModule::GetPostCompile() — retained for unsubscription.
        FDelegateHandle GPostCompileHandle;

        // Names of descriptors we've pushed into the FProcessorRegistry on behalf of script classes. Tracked
        // so the stale set can be deregistered before each re-registration pass.
        TArray<FName> GRegisteredDescriptorNames;

        // Resolves a friendly group name (e.g. "FGroup_Gameplay_Script") to the canonical FName the
        // graph builder actually knows (entt::type_name gives "struct ck::FGroup_Gameplay_Script" on
        // MSVC, "ck::FGroup_Gameplay_Script" on Clang). Matches only at a type-name boundary so
        // "Gameplay_Script" does not collide with "MetaGameplay_Script".
        auto
        DoResolveGroupName(
            FName InFriendlyName)
            -> FName
        {
            if (InFriendlyName == NAME_None)
            { return NAME_None; }

            const auto FriendlyStr = InFriendlyName.ToString();

            const auto IsBoundaryChar = [](TCHAR InChar) -> bool
            {
                return InChar == TEXT(':') || InChar == TEXT(' ');
            };

            for (const auto& Desc : FProcessorRegistry::Get().Get_AllDescriptors())
            {
                const auto CanonicalStr = Desc._Name.ToString();

                if (CanonicalStr.Equals(FriendlyStr))
                { return Desc._Name; }

                if (NOT CanonicalStr.EndsWith(FriendlyStr))
                { continue; }

                const auto SuffixStart = CanonicalStr.Len() - FriendlyStr.Len();
                if (SuffixStart == 0 || IsBoundaryChar(CanonicalStr[SuffixStart - 1]))
                { return Desc._Name; }
            }

            // No match — return as-is, which will produce the graph builder's "not registered" warning.
            return InFriendlyName;
        }

        // True iff InClass (or an intermediate script class) overrides the named base BlueprintImplementableEvent.
        // Script overrides of BIEs materialize as UFunctions on the script class, so a non-override resolves to the
        // base's UFunction (outer == base) while an override resolves to one whose outer is the script class.
        auto
        DoOverridesEvent(
            UClass* InClass,
            FName InEventName)
            -> bool
        {
            const auto* Fn = InClass->FindFunctionByName(InEventName);
            return Fn != nullptr && Fn->GetOuterUClass() != UCk_Processor_Script_Base_UE::StaticClass();
        }

        // Fill the fields common to both hosting paths: name (always the DEV class path name so RunAfter references
        // between processors keep working), group, RunAfter/RunBefore ordering edges, and the MarkedDirtyBy pump
        // gate. Ordering lives here (not on the typed path only) so a processor keeps the same scheduler edges
        // whether it routes legacy or typed — including the two-pass window where its driver is not generated yet.
        auto
        DoFillCommon(
            FProcessorDescriptor& OutDescriptor,
            UClass* InDevClass,
            const UCk_Processor_Script_Base_UE* InCDO)
            -> void
        {
            OutDescriptor._Name = FName{*InDevClass->GetPathName()};
            OutDescriptor._GroupName = DoResolveGroupName(InCDO->Get_Group());

            for (const auto& RunAfterName : InCDO->Get_RunAfter())
            {
                OutDescriptor._RunAfter.Add(DoResolveGroupName(RunAfterName));
            }
            for (const auto& RunBeforeName : InCDO->Get_RunBefore())
            {
                OutDescriptor._RunBefore.Add(DoResolveGroupName(RunBeforeName));
            }

            if (auto* MarkedDirtyBy = InCDO->Get_MarkedDirtyBy().Get())
            {
                OutDescriptor._HasDirtyMarker = true;
                const auto DirtyMarkerName = FName{*MarkedDirtyBy->GetPathName()};
                OutDescriptor._DirtyMarkerNames.Add(DirtyMarkerName);

                // MUST be the hash the dynamic-fragment mutation paths bump (both sides share this
                // helper) — the scheduler's persistent version short-circuit compares the two. A
                // divergent hash domain here means Get_DirtyMarkerVersion never advances and the
                // node goes pump-deaf after its first evaluation.
                OutDescriptor._DirtyMarkerHashes.Add(UCk_Utils_DynamicFragment_UE::Get_DirtyMarkerHash(MarkedDirtyBy));

                OutDescriptor._IsDirtyChecker =
                    [WeakStruct = TWeakObjectPtr<UScriptStruct>(MarkedDirtyBy)]
                    (const FCk_Registry& InRegistry) -> bool
                    {
                        const auto* Struct = WeakStruct.Get();
                        if (Struct == nullptr)
                        { return false; }

                        // Reach the ECS storage through the registry's transient entity — any handle from the
                        // same registry resolves to the same underlying entt storage pool.
                        const auto TransientHandle =
                            UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InRegistry);

                        return UCk_Utils_DynamicFragment_UE::Has_AnyEntityWith_Fragment(TransientHandle, Struct);
                    };
            }
        }

        // Typed hosting path (FProcessor_ScriptQueryHosted): a native join drives one ForEachBatch call. InDriverClass
        // is the generated <Dev>_Driver (a SUBCLASS of the dev class), or nullptr for direct mode (the dev class
        // overrides ForEachBatch itself). The read/write fragment sets that feed scheduler conflict-detection +
        // auto-ordering come from the query, obtained by calling Configure on the batch class's CDO — the generated
        // override adds the inferred slots and (when the dev overrides Configure) Super::Configure contributes the
        // dev's Require/Exclude, so the harvest sees the full query.
        auto
        DoBuildQueryDescriptor(
            UClass* InDevClass,
            const UCk_Processor_Script_Base_UE* InDevCDO,
            UClass* InDriverClass)
            -> FProcessorDescriptor
        {
            auto Descriptor = FProcessorDescriptor{};
            DoFillCommon(Descriptor, InDevClass, InDevCDO);

            auto* BatchClass = ck::IsValid(InDriverClass) ? InDriverClass : InDevClass;
            auto* BatchCDO = Cast<UCk_Processor_Script_Base_UE>(BatchClass->GetDefaultObject());

            auto Query = FCk_ScriptProcessorQuery{};
            if (ck::IsValid(BatchCDO))
            {
                BatchCDO->Configure(Query);
            }

            for (const auto& Slot : Query._Slots)
            {
                const auto* Type = Slot._StructType.Get();
                if (ck::Is_NOT_Valid(Type))
                { continue; }

                const auto FragmentName = FName{*Type->GetPathName()};
                const auto FragmentHash = static_cast<uint32>(GetTypeHash(FragmentName));

                switch (Slot._Access)
                {
                    case ECk_ScriptQueryAccess::ReadWrite:
                    {
                        Descriptor._RW_FragmentHashes.Add(FragmentHash);
                        Descriptor._RW_FragmentNames.Add(FragmentName);
                        break;
                    }
                    case ECk_ScriptQueryAccess::ReadOnly:
                    case ECk_ScriptQueryAccess::Require:
                    {
                        Descriptor._RO_FragmentHashes.Add(FragmentHash);
                        Descriptor._RO_FragmentNames.Add(FragmentName);
                        break;
                    }
                    case ECk_ScriptQueryAccess::Exclude:
                    {
                        break;   // neither read nor write set
                    }
                }
            }

            Descriptor._Factory =
                [DevClass = InDevClass, DriverClass = InDriverClass](const FCk_Registry& InRegistry) -> concepts::FTickableType
                {
                    return FProcessor_ScriptQueryHosted{InRegistry, DevClass, DriverClass};
                };

            return Descriptor;
        }

        auto
        DoRegisterAllScriptProcessors()
            -> void
        {
            // Deregister the previous pass before re-discovering — makes this call idempotent and safe
            // to call repeatedly (initial build, live rebuild after AS reload, etc.).
            for (const auto& Name : GRegisteredDescriptorNames)
            {
                FProcessorRegistry::Get().Deregister(Name);
            }
            GRegisteredDescriptorNames.Reset();

            auto DiscoveredClasses = TArray<UClass*>{};
            GetDerivedClasses(UCk_Processor_Script_Base_UE::StaticClass(), DiscoveredClasses, /*bRecursive=*/true);

            // Name -> class lookup for driver resolution: a typed processor's driver is a sibling subclass named
            // <Dev>_Driver, discovered in the same set and never registered on its own.
            auto ClassByName = TMap<FString, UClass*>{};
            for (auto* Class : DiscoveredClasses)
            {
                if (ck::IsValid(Class))
                { ClassByName.Add(Class->GetName(), Class); }
            }

            const auto DriverSuffix = FString{TEXT("_Driver")};
            const auto ForEachBatchName = FName{TEXT("ForEachBatch")};

            for (auto* Class : DiscoveredClasses)
            {
                if (ck::Is_NOT_Valid(Class))
                { continue; }

                if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
                { continue; }

                // Drivers are registered via their dev class, never standalone. Only a class whose dev sibling
                // actually exists is a driver — an ordinary processor whose name merely ends in _Driver falls
                // through and registers normally.
                if (Class->GetName().EndsWith(DriverSuffix) &&
                    ClassByName.Contains(Class->GetName().LeftChop(DriverSuffix.Len())))
                { continue; }

                const auto* CDO = Cast<UCk_Processor_Script_Base_UE>(Class->GetDefaultObject());
                if (ck::Is_NOT_Valid(CDO))
                { continue; }

                auto Descriptor = FProcessorDescriptor{};

                if (auto DriverEntry = ClassByName.Find(Class->GetName() + DriverSuffix))
                {
                    // Typed processor with a generated (or hand-written) driver.
                    Descriptor = DoBuildQueryDescriptor(Class, CDO, *DriverEntry);
                }
                else if (DoOverridesEvent(Class, ForEachBatchName))
                {
                    // Direct mode: the class overrides ForEachBatch itself (no separate driver class).
                    Descriptor = DoBuildQueryDescriptor(Class, CDO, nullptr);
                }
                else
                {
                    // Neither a generated <Dev>_Driver nor a ForEachBatch override exists yet. Two cases, both
                    // "nothing to register now":
                    //   - a typed processor authored THIS compile whose driver has not been generated yet — the
                    //     driver generator runs in the same PostCompile and its file-write triggers a re-compile +
                    //     re-registration, at which point the driver is found above (one-compile window).
                    //   - a lifecycle-only processor that never declared a dispatch (no ForEachBatch / NoEntities) —
                    //     there is no iteration surface to host.
                    ck::ecs::Verbose(TEXT("Script processor [{}] has no generated driver and no ForEachBatch override "
                        "yet — skipped (typed processor pending driver generation, or a lifecycle-only class with no "
                        "dispatch)."), Class->GetName());
                    continue;
                }

                const auto DescriptorName = Descriptor._Name;

                FProcessorRegistry::Get().Register(MoveTemp(Descriptor));
                GRegisteredDescriptorNames.Add(DescriptorName);

                ck::ecs::Verbose(TEXT("Registered script processor [{}]"), DescriptorName);
            }
        }

        auto
        DoRequestRebuildOnAllWorlds()
            -> void
        {
            if (NOT GEngine)
            { return; }

            for (const auto& WorldContext : GEngine->GetWorldContexts())
            {
                auto* World = WorldContext.World();
                if (ck::Is_NOT_Valid(World))
                { continue; }

                auto* Subsystem = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
                if (ck::Is_NOT_Valid(Subsystem))
                { continue; }

                Subsystem->Request_RebuildProcessorGraph();
            }
        }
    }

    auto
        FScriptProcessor_Host::
        Startup()
        -> void
    {
        GOnPreBuildHandle = UCk_EcsWorld_Subsystem_UE::Get_OnPreBuildProcessorGraph().AddLambda(
            [](UWorld& /*InWorld*/)
            {
                DoRegisterAllScriptProcessors();
            });

#if WITH_ANGELSCRIPT_CK
        GPostCompileHandle = FAngelscriptCodeModule::GetPostCompile().AddLambda([]()
        {
            DoRequestRebuildOnAllWorlds();
        });
#endif
    }

    auto
        FScriptProcessor_Host::
        Shutdown()
        -> void
    {
#if WITH_ANGELSCRIPT_CK
        FAngelscriptCodeModule::GetPostCompile().Remove(GPostCompileHandle);
        GPostCompileHandle.Reset();
#endif

        UCk_EcsWorld_Subsystem_UE::Get_OnPreBuildProcessorGraph().Remove(GOnPreBuildHandle);
        GOnPreBuildHandle.Reset();

        for (const auto& Name : GRegisteredDescriptorNames)
        {
            FProcessorRegistry::Get().Deregister(Name);
        }
        GRegisteredDescriptorNames.Reset();
    }
}
