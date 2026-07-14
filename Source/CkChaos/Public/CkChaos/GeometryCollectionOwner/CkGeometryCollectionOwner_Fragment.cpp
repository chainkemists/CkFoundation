#include "CkGeometryCollectionOwner_Fragment.h"

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment.h"
#include "CkChaos/GeometryCollection/CkGeometryCollection_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

#include "TargetPoint/CkTargetPoint_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for GeometryCollectionOwner

static struct FGeometryCollectionOwnerRepHandlerRegistrar
{
    FGeometryCollectionOwnerRepHandlerRegistrar()
    {
        const auto DoApply = [](FCk_Handle& Entity, const FCk_RepData_GeometryCollectionOwner& NewData, const FCk_RepData_GeometryCollectionOwner& OldData)
        {
            if (ck::Is_NOT_Valid(Entity))
            { return; }

            if (NewData.CrumbleNonActiveClustersRequest != OldData.CrumbleNonActiveClustersRequest)
            {
                ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
                {
                    UCk_Utils_GeometryCollection_UE::Request_CrumbleNonAnchoredClusters(InGc);
                });
            }

            if (NewData.RemoveAllAnchors != OldData.RemoveAllAnchors)
            {
                ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
                {
                    UCk_Utils_GeometryCollection_UE::Request_RemoveAllAnchors(InGc);
                });
            }

            if (NewData.RemoveAllAnchorsAndCrumbleNonActiveClusters != OldData.RemoveAllAnchorsAndCrumbleNonActiveClusters)
            {
                ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
                {
                    UCk_Utils_GeometryCollection_UE::Request_RemoveAllAnchors(InGc);
                    UCk_Utils_GeometryCollection_UE::Request_CrumbleNonAnchoredClusters(InGc);
                });
            }

            for (auto Index = OldData.RadialStrains.Num(); Index < NewData.RadialStrains.Num(); ++Index)
            {
                ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
                {
                    const auto& Request = NewData.RadialStrains[Index];
                    UCk_Utils_GeometryCollection_UE::Request_ApplyRadialStrain(InGc, FCk_Request_GeometryCollection_ApplyRadialStrain
                        {
                            Request.Get_Location(),
                            Request.Get_Request()->Get_Radius()
                        }
                        .Set_InternalStrain(Request.Get_Request()->Get_InternalStrain())
                        .Set_ExternalStrain(Request.Get_Request()->Get_ExternalStrain())
                        .Set_LinearSpeed(Request.Get_Request()->Get_LinearSpeed())
                        .Set_AngularSpeed(Request.Get_Request()->Get_AngularSpeed())
                        .Set_ChangeParticleStateTo(Request.Get_Request()->Get_ChangeParticleStateTo())
                        .Set_NormalizedFalloffCurve(Request.Get_Request()->Get_NormalizedFalloffCurve())
                    );
                });
            }
        };

        FCk_PersistenceHandlerRegistry::Register_NetOnly<FCk_RepData_GeometryCollectionOwner>({
                // Event-edge semantics (crumble/anchor/strain requests fire on field deltas vs the
                // last APPLIED data) — always Applied; a not-yet-populated GC record just means the
                // events have no targets, same as the old inline path.
                .NetApply = [DoApply](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    DoApply(Entity,
                        New.Get<FCk_RepData_GeometryCollectionOwner>(),
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_GeometryCollectionOwner>()
                            : FCk_RepData_GeometryCollectionOwner{});
                    return ECk_Persistence_ApplyResult::Applied;
                }
            });
    }
} GGeometryCollectionOwnerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
