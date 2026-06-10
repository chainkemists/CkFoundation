#include "CkStateMachine_Fragment.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "CkEcs/Handle/CkDebugCallstack_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include <Serialization/Archive.h>
#include <UObject/SoftObjectPath.h>

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKSTATEMACHINE_API, statemachine, ck::FFragment_Sm_Requests);

// --------------------------------------------------------------------------------------------------------------------
// Snapshot serialize bodies. Out-of-line (same module) so TryLoadClass<UCk_SmState_EntityScript> sees the
// complete type without dragging the EntityScript header into CkStateMachine_Fragment.h.
//
// Class fields persist by FSoftClassPath{Class}.ToString() — i.e. GetPathName — which is the only form that
// round-trips AngelScript-authored state classes (their UClass names strip the leading U under
// /Script/Angelscript; a hand-formed path would silently TryLoadClass to null).

namespace ck
{
    auto
        FFragment_Sm_Current::
        SerializeSnapshot(
            FArchive& InAr,
            ck::FSnapshotContext& /*InCtx*/)
        -> void
    {
        auto RunStatusByte = uint8{0};
        auto CurrentStateClassPath = FString{};

        if (InAr.IsSaving())
        {
            RunStatusByte = static_cast<uint8>(_RunStatus);
            if (ck::IsValid(_CurrentStateClass))
            { CurrentStateClassPath = FSoftClassPath{_CurrentStateClass.Get()}.ToString(); }

            // Mid-flight transition at capture (FFragment_Sm_PendingTransition present): the class
            // saved here is null (DoExitCurrentState already cleared it) and the in-flight TARGET is
            // not persisted — the restored machine re-drives into InitialState only. Known v1 scope
            // cut; persisting the target needs a capture-side pending-transition flush (the snapshot
            // serializer cannot see sibling fragments, and restoring a PendingTransition races
            // FProcessor_Sm_CommitPendingTransition before the driver-gated re-drive can strip it).
        }

        InAr << RunStatusByte;
        InAr << CurrentStateClassPath;

        if (InAr.IsLoading())
        {
            _RunStatus = static_cast<ECk_SmRunStatus>(RunStatusByte);
            _CurrentStateHandle = FCk_Handle_SmState{};
            _CurrentStateClass = CurrentStateClassPath.IsEmpty()
                ? nullptr
                : FSoftClassPath{CurrentStateClassPath}.TryLoadClass<UCk_SmState_EntityScript>();
        }

        ck::sm::Verbose(TEXT("[SM Snapshot] Sm_Current {}: RunStatus [{}] class [{}]"),
            InAr.IsSaving() ? TEXT("SAVE") : TEXT("LOAD"),
            static_cast<int32>(RunStatusByte), CurrentStateClassPath);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FFragment_Sm_ParentHierarchy::
        SerializeSnapshot(
            FArchive& InAr,
            ck::FSnapshotContext& /*InCtx*/)
        -> void
    {
        auto TagNames = TArray<FName>{};

        if (InAr.IsSaving())
        {
            for (const auto& Tag : _ParentHierarchy)
            { TagNames.Add(Tag.GetTagName()); }
        }

        InAr << TagNames;

        if (InAr.IsLoading())
        {
            constexpr auto ErrorIfNotFound = false;

            _ParentHierarchy.Reset();
            for (const auto& TagName : TagNames)
            { _ParentHierarchy.Add(FGameplayTag::RequestGameplayTag(TagName, ErrorIfNotFound)); }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FFragment_Sm_StateOverrides::
        SerializeSnapshot(
            FArchive& InAr,
            ck::FSnapshotContext& /*InCtx*/)
        -> void
    {
        auto Num = int32{0};

        if (InAr.IsSaving())
        { Num = _Overrides.Num(); }

        InAr << Num;

        if (InAr.IsLoading())
        { _Overrides.SetNum(Num); }

        for (auto Index = 0; Index < Num; ++Index)
        {
            auto& Entry = _Overrides[Index];

            auto ClassPath = FString{};
            auto TagNames = TArray<FName>{};

            if (InAr.IsSaving())
            {
                if (ck::IsValid(Entry._OverrideStateClass))
                { ClassPath = FSoftClassPath{Entry._OverrideStateClass.Get()}.ToString(); }

                for (const auto& Tag : Entry._CachedStatesToOverride)
                { TagNames.Add(Tag.GetTagName()); }
            }

            InAr << ClassPath;
            InAr << TagNames;

            if (InAr.IsLoading())
            {
                constexpr auto ErrorIfNotFound = false;

                Entry._OverrideStateClass = ClassPath.IsEmpty()
                    ? nullptr
                    : FSoftClassPath{ClassPath}.TryLoadClass<UCk_SmState_EntityScript>();

                Entry._CachedStatesToOverride.Reset();
                for (const auto& TagName : TagNames)
                { Entry._CachedStatesToOverride.Add(FGameplayTag::RequestGameplayTag(TagName, ErrorIfNotFound)); }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Snapshot registrations (aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the name).
// FCk_Fragment_StateMachine_ParamsData is Tier-A (SaveGame metas, used directly as FFragment_Sm_Params);
// the ck:: fragments are Tier-C via the SerializeSnapshot bodies above.

using FSnap_Sm_Current         = ck::FFragment_Sm_Current;
using FSnap_Sm_ParentHierarchy = ck::FFragment_Sm_ParentHierarchy;
using FSnap_Sm_StateOverrides  = ck::FFragment_Sm_StateOverrides;

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_StateMachine_ParamsData);
CK_REGISTER_SNAPSHOTABLE(FSnap_Sm_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_Sm_ParentHierarchy);
CK_REGISTER_SNAPSHOTABLE(FSnap_Sm_StateOverrides);

// --------------------------------------------------------------------------------------------------------------------
