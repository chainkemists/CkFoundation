#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Concepts/CkSnapshot_Concepts.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"

#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp"

namespace ck
{
    class CKECS_API FSnapshotArchive_Reader
    {
    public:
        FSnapshotArchive_Reader(FObjectAndNameAsStringProxyArchive& InProxy, FSnapshotContext& InContext)
            : _Proxy(InProxy), _Context(InContext)
        {
            // MUST mirror FSnapshotArchive_Writer exactly (see there for the rationale): whole-fragment capture,
            // Transient is the only opt-out. Any writer/reader flag asymmetry desyncs the byte stream.
            _Proxy.ArIsSaveGame = false;
            _Proxy.SetIsPersistent(true);
        }

        auto operator()(entt::entity& OutEntity) -> void;
        auto operator()(std::underlying_type_t<entt::entity>& OutSize) -> void;

        template <typename T> requires ck::concepts::FragmentIsSnapshotable<T>
        auto operator()(T& OutFragment) -> void
        {
            ck::detail::DoSerializeSnapshot_OneInstance(_Proxy, _Context, OutFragment);
        }

    private:
        FObjectAndNameAsStringProxyArchive& _Proxy;
        FSnapshotContext&                   _Context;
    };
}
