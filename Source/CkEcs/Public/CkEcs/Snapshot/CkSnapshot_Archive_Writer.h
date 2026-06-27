#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Concepts/CkSnapshot_Concepts.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"

#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp"

namespace ck
{
    class CKECS_API FSnapshotArchive_Writer
    {
    public:
        FSnapshotArchive_Writer(FObjectAndNameAsStringProxyArchive& InProxy, FSnapshotContext& InContext)
            : _Proxy(InProxy), _Context(InContext)
        {
            // CkSnapshot captures the WHOLE fragment, not just SaveGame-marked fields -- a snapshot is a complete
            // ECS-world capture, so the persistence contract is "everything EXCEPT Transient". ArIsSaveGame=false
            // disables SaveGame gating (bare UPROPERTY data round-trips; AngelScript fragments cannot reliably set
            // CPF_SaveGame -- they only reach meta=(SaveGame), which is not the real flag). ArIsPersistent=true makes
            // CPF_Transient the single opt-out, applied SYMMETRICALLY with the reader -- a writer/reader flag mismatch
            // would desync the byte stream. (Handles stay out of this data pass via Transient on FCk_Handle's fields
            // and are round-tripped through FSnapshotContext::Snapshot_Handle's entity-id remap instead.)
            _Proxy.ArIsSaveGame = false;
            _Proxy.SetIsPersistent(true);
        }

        auto operator()(entt::entity InEntity) -> void;
        auto operator()(std::underlying_type_t<entt::entity> InSize) -> void;

        template <typename T> requires ck::concepts::FragmentIsSnapshotable<T>
        auto operator()(T& InFragment) -> void
        {
            ck::detail::DoSerializeSnapshot_OneInstance(_Proxy, _Context, InFragment);
        }

    private:
        FObjectAndNameAsStringProxyArchive& _Proxy;
        FSnapshotContext&                   _Context;
    };
}
