#pragma once

#include "CkEcs/Net/CkNet_Fragment_Data.h"
#include "CkEcs/Tag/CkTag.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FSnapshotContext; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_HasAuthority);

    // Defined in CkHandle.h to avoid circular dependency since it's needed for debugging purposes
    //CK_DEFINE_ECS_TAG(FTag_NetMode_IsHost);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_Net_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Net_Params);
        using IsSnapshotable = void;

    private:
        FCk_Net_ConnectionSettings _ConnectionSettings;

    public:
        CK_PROPERTY_GET(_ConnectionSettings);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Net_Params, _ConnectionSettings);

        // Tier C — pure value data (3 enums; no handle refs). INLINE because FFragment_Net_Params is CKECS_API:
        // an out-of-line body defined in CkSnapshot would be inconsistent dll linkage (C4273). InCtx is unused.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto Replication = static_cast<uint8>(_ConnectionSettings.Get_Replication());
            auto NetMode     = static_cast<uint8>(_ConnectionSettings.Get_NetMode());
            auto NetRole     = static_cast<uint8>(_ConnectionSettings.Get_NetRole());

            InAr << Replication;
            InAr << NetMode;
            InAr << NetRole;

            if (InAr.IsLoading())
            {
                _ConnectionSettings = FCk_Net_ConnectionSettings
                {
                    static_cast<ECk_Replication>(Replication),
                    static_cast<ECk_Net_NetModeType>(NetMode),
                    static_cast<ECk_Net_EntityNetRole>(NetRole)
                };
            }
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------
