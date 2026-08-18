#include "CkEntityTag_Fragment.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h"
#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_EntityTag_Root, TEXT("EntityTag"));

// --------------------------------------------------------------------------------------------------------------------

static struct FCkEntityTagSaveHandlerRegistrar
{
    FCkEntityTagSaveHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_EntityTags>({
            .Posture = ECk_Snapshot_Posture::Durable,
            .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
            {
                if (NOT Entity.Has<ck::FFragment_EntityTag_Current>())
                { return {}; }

                const auto& Tags = Entity.Get<ck::FFragment_EntityTag_Current>().Get_Tags();
                if (Tags.IsEmpty())
                { return {}; }

                auto Payload = FCk_SaveData_EntityTags{};
                Payload.Get_TagNames().Reserve(Tags.Num());
                Payload.Get_Counts().Reserve(Tags.Num());
                for (const auto& TagCount : Tags)
                {
                    Payload.Get_TagNames().Add(TagCount._Name);
                    Payload.Get_Counts().Add(TagCount._Count);
                }
                return FInstancedStruct::Make(Payload);
            },
            // Reading the live tag set here would miss the GatedDuringLoad construct-seeds still enqueued-but-undrained,
            // so this enqueues ONE composite request that diffs at DRAIN time instead — see CkEntityTag/CLAUDE.md
            // § "Save/load restore".
            .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
            {
                CK_ENSURE_IF_NOT(ck::IsValid(Entity),
                    TEXT("EntityTag hydration skipped — invalid entity [{}]"), Entity)
                { return ECk_Persistence_ApplyResult::Applied; }

                const auto& Payload     = New.Get<FCk_SaveData_EntityTags>();
                const auto& SavedNames  = Payload.Get_TagNames();
                const auto& SavedCounts = Payload.Get_Counts();

                CK_ENSURE_IF_NOT(SavedNames.Num() == SavedCounts.Num(),
                    TEXT("EntityTag hydration payload malformed on [{}] — [{}] names vs [{}] counts; restoring the common prefix"),
                    Entity, SavedNames.Num(), SavedCounts.Num())
                { /* DoApply_RestoreSet clamps to the common prefix rather than restoring nothing */ }

                UCk_Utils_EntityTag_UE::Request_RestoreSet(Entity, SavedNames, SavedCounts, {});

                return ECk_Persistence_ApplyResult::Applied;
            }});
    }
} GCkEntityTagSaveHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
