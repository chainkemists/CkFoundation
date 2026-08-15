#include "CkDynamic_ScriptProcessor_Host.h"

#include "CkDynamic_Utils.h"
#include "CkDynamic_ScriptQueryProcessor.h"

#include "CkCore/Format/CkFormat.h"
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
    namespace ck_dynamic_script_processor_host
    {
        FDelegateHandle GOnPreBuildHandle;
        FDelegateHandle GPostCompileHandle;
        TArray<FName>   GRegisteredDescriptorNames;

        // entt::type_name yields "struct ck::FGroup_X" on MSVC and "ck::FGroup_X" on Clang, so the canonical
        // FName is matched by suffix — at a type-name boundary, or "Gameplay_Script" collides with
        // "MetaGameplay_Script".
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

        // Script overrides of a BlueprintImplementableEvent materialize as UFunctions on the script class, so a
        // non-override resolves to the base's UFunction while an override's outer is the script class.
        auto
        DoOverridesEvent(
            UClass* InClass,
            FName InEventName)
            -> bool
        {
            const auto* Fn = InClass->FindFunctionByName(InEventName);
            return Fn != nullptr && Fn->GetOuterUClass() != UCk_Processor_Script_Base_UE::StaticClass();
        }

        auto
        DoFillCommon(
            FProcessorDescriptor& OutDescriptor,
            UClass* InDevClass,
            const UCk_Processor_Script_Base_UE* InCDO)
            -> void
        {
            OutDescriptor._Name = FName{*InDevClass->GetPathName()};

            // _Name stays the full path so RunAfter references resolve; this is the short trace identity.
            OutDescriptor._DisplayName = FName{*ck::Format_UE(TEXT("script::{}"), InDevClass->GetName())};

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

                OutDescriptor._DirtyMarkerHashes.Add(UCk_Utils_DynamicFragment_UE::Get_DirtyMarkerHash(MarkedDirtyBy));

                OutDescriptor._IsDirtyChecker =
                    [WeakStruct = TWeakObjectPtr<UScriptStruct>(MarkedDirtyBy)]
                    (const FCk_Registry& InRegistry) -> bool
                    {
                        const auto* Struct = WeakStruct.Get();
                        if (Struct == nullptr)
                        { return false; }

                        // Any handle from the same registry resolves to the same underlying entt storage pool.
                        const auto TransientHandle =
                            UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InRegistry);

                        return UCk_Utils_DynamicFragment_UE::Has_AnyLiveEntityWith_Fragment(TransientHandle, Struct);
                    };
            }
        }

        // InDriverClass is the generated <Dev>_Driver (a SUBCLASS of the dev class), or nullptr for direct mode.
        // Configure MUST run on the BATCH class's CDO for the harvest to see the full query.
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

            // Keep the factory present even when descriptor admission fails. The hosted processor repeats validation
            // and disables itself, while this descriptor advertises no partial read/write contract.
            Descriptor._Factory =
                [DevClass = InDevClass, DriverClass = InDriverClass](const FCk_Registry& InRegistry) -> concepts::FTickableType
                {
                    return FProcessor_ScriptQueryHosted{InRegistry, DevClass, DriverClass};
                };

            const auto AdmissionSucceeded = NOT Query._AdmissionFailed;
            CK_ENSURE_IF_NOT(AdmissionSucceeded,
                TEXT("Script processor descriptor [{}] rejected a partially admitted query"), InDevClass)
            { return Descriptor; }

            const auto QueryContext = BatchClass->GetPathName();
            if (NOT ck::dynamic::Validate_ScriptQuerySlots(Query, *QueryContext))
            { return Descriptor; }

            for (const auto& Slot : Query._Slots)
            {
                const auto* Type = Slot._StructType.Get();

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

            return Descriptor;
        }

        auto
        DoRegisterAllScriptProcessors()
            -> void
        {
            for (const auto& Name : GRegisteredDescriptorNames)
            {
                FProcessorRegistry::Get().Deregister(Name);
            }
            GRegisteredDescriptorNames.Reset();

            auto DiscoveredClasses = TArray<UClass*>{};
            GetDerivedClasses(UCk_Processor_Script_Base_UE::StaticClass(), DiscoveredClasses, /*bRecursive=*/true);

            // A typed processor's driver is a sibling subclass named <Dev>_Driver, discovered in this same set.
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

                // Drivers register via their dev class, never standalone. The sibling-exists half of the test is
                // what lets an ordinary processor whose name merely ends in _Driver fall through and register.
                if (Class->GetName().EndsWith(DriverSuffix) &&
                    ClassByName.Contains(Class->GetName().LeftChop(DriverSuffix.Len())))
                { continue; }

                const auto* CDO = Cast<UCk_Processor_Script_Base_UE>(Class->GetDefaultObject());
                if (ck::Is_NOT_Valid(CDO))
                { continue; }

                auto Descriptor = FProcessorDescriptor{};

                if (auto DriverEntry = ClassByName.Find(Class->GetName() + DriverSuffix))
                {
                    Descriptor = DoBuildQueryDescriptor(Class, CDO, *DriverEntry);
                }
                else if (DoOverridesEvent(Class, ForEachBatchName))
                {
                    constexpr UClass* DirectMode_NoDriverClass = nullptr;
                    Descriptor = DoBuildQueryDescriptor(Class, CDO, DirectMode_NoDriverClass);
                }
                else
                {
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
        ck_dynamic_script_processor_host::GOnPreBuildHandle = UCk_EcsWorld_Subsystem_UE::Get_OnPreBuildProcessorGraph().AddLambda(
            [](UWorld& /*InWorld*/)
            {
                ck_dynamic_script_processor_host::DoRegisterAllScriptProcessors();
            });

#if WITH_ANGELSCRIPT_CK
        ck_dynamic_script_processor_host::GPostCompileHandle = FAngelscriptCodeModule::GetPostCompile().AddLambda([]()
        {
            ck_dynamic_script_processor_host::DoRequestRebuildOnAllWorlds();
        });
#endif
    }

    auto
        FScriptProcessor_Host::
        Shutdown()
        -> void
    {
#if WITH_ANGELSCRIPT_CK
        FAngelscriptCodeModule::GetPostCompile().Remove(ck_dynamic_script_processor_host::GPostCompileHandle);
        ck_dynamic_script_processor_host::GPostCompileHandle.Reset();
#endif

        UCk_EcsWorld_Subsystem_UE::Get_OnPreBuildProcessorGraph().Remove(ck_dynamic_script_processor_host::GOnPreBuildHandle);
        ck_dynamic_script_processor_host::GOnPreBuildHandle.Reset();

        for (const auto& Name : ck_dynamic_script_processor_host::GRegisteredDescriptorNames)
        {
            FProcessorRegistry::Get().Deregister(Name);
        }
        ck_dynamic_script_processor_host::GRegisteredDescriptorNames.Reset();
    }
}
