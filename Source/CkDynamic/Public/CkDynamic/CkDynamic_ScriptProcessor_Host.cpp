#include "CkDynamic_ScriptProcessor_Host.h"

#include "CkDynamic_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Processor/CkProcessor_Script.h"
#include "CkEcs/Processor/CkProcessor_ScriptHosted.h"
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

        auto
        DoBuildDescriptorForClass(
            UClass* InClass)
            -> TOptional<FProcessorDescriptor>
        {
            if (ck::Is_NOT_Valid(InClass))
            { return {}; }

            if (InClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
            { return {}; }

            const auto* CDO = Cast<UCk_Processor_Script_Base_UE>(InClass->GetDefaultObject());
            if (ck::Is_NOT_Valid(CDO))
            { return {}; }

            auto Descriptor = FProcessorDescriptor{};

            Descriptor._Name = FName{*InClass->GetPathName()};
            Descriptor._GroupName = DoResolveGroupName(CDO->Get_Group());

            // Factory spawns a FProcessor_ScriptHosted bound to this class. Captured by value so the closure is
            // independent of anything outside the registry's lifetime.
            Descriptor._Factory = [ScriptClass = InClass](const FCk_Registry& InRegistry) -> concepts::FTickableType
            {
                return FProcessor_ScriptHosted{InRegistry, ScriptClass};
            };

            if (auto* MarkedDirtyBy = CDO->Get_MarkedDirtyBy().Get())
            {
                Descriptor._HasDirtyMarker = true;
                const auto DirtyMarkerName = FName{*MarkedDirtyBy->GetPathName()};
                Descriptor._DirtyMarkerNames.Add(DirtyMarkerName);
                Descriptor._DirtyMarkerHashes.Add(static_cast<uint32>(GetTypeHash(DirtyMarkerName)));

                Descriptor._IsDirtyChecker =
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

            for (auto* Class : DiscoveredClasses)
            {
                auto DescriptorOpt = DoBuildDescriptorForClass(Class);
                if (NOT DescriptorOpt.IsSet())
                { continue; }

                auto DescriptorName = DescriptorOpt.GetValue()._Name;

                FProcessorRegistry::Get().Register(MoveTemp(DescriptorOpt.GetValue()));
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
