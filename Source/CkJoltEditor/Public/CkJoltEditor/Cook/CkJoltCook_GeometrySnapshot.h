#pragma once

#include "CkJolt/Query/CkJoltOccupancy_Session.h"

#include <CoreMinimal.h>

class AActor;

namespace ck::jolt::cook
{
    /**
     * Value-only static geometry captured while a World Partition descriptor actor is loaded. The snapshot
     * owns no actor, component, Jolt shape, or descriptor pointers, so it remains valid after the helper
     * releases that actor's loading batch.
     */
    struct CKJOLTEDITOR_API FCk_Jolt_CookGeometryBody
    {
        FCk_Jolt_TriangleSoup _Triangles;
        FBox _Bounds = FBox{ForceInit};
        ECk_Jolt_StaticBodyKind _Kind = ECk_Jolt_StaticBodyKind::Solid;
        FString _Description;
        TArray<FName> _DataLayerNames;
    };

    class CKJOLTEDITOR_API FCk_Jolt_CookGeometrySnapshot
    {
    public:
        /** Extracts one actor under the production LevelSweep/filter policy and copies all geometry to values. */
        auto Try_AddActor(const AActor& InActor) -> bool;
        auto Get_Bodies() const -> const TArray<FCk_Jolt_CookGeometryBody>& { return _Bodies; }
        auto Get_NumActors() const -> int32 { return _NumActors; }
        auto Get_IsComplete() const -> bool { return _bComplete; }

    private:
        TArray<FCk_Jolt_CookGeometryBody> _Bodies;
        int32 _NumActors = 0;
        bool _bComplete = true;
    };
}