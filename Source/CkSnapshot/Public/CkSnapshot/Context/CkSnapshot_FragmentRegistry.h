#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Concepts/CkSnapshot_Concepts.h"

#include "CkSnapshot/Context/CkSnapshot_Context.h"

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp"
#include "CkThirdParty/entt-3.16.0/src/entt/entity/snapshot.hpp"

#include <functional>

class UScriptStruct;
class FArchive;

namespace ck
{
    struct CKSNAPSHOT_API FCk_Snapshot_RegisteredFragment
    {
        FString  _DisplayName;
        uint32   _EnttTypeHash = 0;

        std::function<void(entt::basic_snapshot<entt::registry>&,           FArchive&, FSnapshotContext&)> _Save;
        std::function<void(entt::basic_continuous_loader<entt::registry>&,  FArchive&, FSnapshotContext&)> _Load;

        UScriptStruct* _ScriptStruct = nullptr;
    };

    class CKSNAPSHOT_API FCk_Snapshot_FragmentRegistry
    {
    public:
        static auto Get() -> FCk_Snapshot_FragmentRegistry&;

        auto Register(FCk_Snapshot_RegisteredFragment InEntry) -> void;
        auto Get_All() const -> const TArray<FCk_Snapshot_RegisteredFragment>&;
        auto Find_ByDisplayName(const FString& InName) const -> const FCk_Snapshot_RegisteredFragment*;
        auto Find_ByEnttHash(uint32 InHash) const -> const FCk_Snapshot_RegisteredFragment*;

    private:
        TArray<FCk_Snapshot_RegisteredFragment> _Entries;
    };

    namespace detail
    {
        template <typename T>
            requires ck::concepts::FragmentHasCustomSnapshotSerialize<T>
        auto DoSerializeSnapshot_OneInstance(FArchive& InAr, FSnapshotContext& InCtx, T& InFragment) -> void
        {
            InFragment.SerializeSnapshot(InAr, InCtx);
        }

        template <typename T>
            requires ck::concepts::FragmentIsUStructSnapshotable<T>
        auto DoSerializeSnapshot_OneInstance(FArchive& InAr, FSnapshotContext& /*InCtx*/, T& InFragment) -> void
        {
            T::StaticStruct()->SerializeItem(InAr, &InFragment, /*Defaults=*/nullptr);
        }

        template <typename T>
        auto Do_RegisterSnapshotable(const TCHAR* InDisplayName) -> void
        {
            FCk_Snapshot_RegisteredFragment Entry;
            Entry._DisplayName  = InDisplayName;
            Entry._EnttTypeHash = entt::type_hash<T>::value();

            if constexpr (ck::concepts::FragmentIsUStructSnapshotable<T>)
            {
                Entry._ScriptStruct = T::StaticStruct();
            }

            Entry._Save = [](entt::basic_snapshot<entt::registry>& InSnap, FArchive&, FSnapshotContext&) -> void
            {
                InSnap.get<T>();
            };

            Entry._Load = [](entt::basic_continuous_loader<entt::registry>& InLoader, FArchive&, FSnapshotContext&) -> void
            {
                InLoader.get<T>();
            };

            FCk_Snapshot_FragmentRegistry::Get().Register(MoveTemp(Entry));
        }
    }
}

// CK_REGISTER_SNAPSHOTABLE(T) -- one line in T's *_Fragment.cpp.
//
// static_assert fires with a precise message if T satisfies neither tier.
#define CK_REGISTER_SNAPSHOTABLE(_FragmentType_) \
    static_assert( \
        ck::concepts::FragmentHasCustomSnapshotSerialize<_FragmentType_> || \
        ck::concepts::FragmentIsUStructSnapshotable<_FragmentType_>, \
        "Fragment " #_FragmentType_ " is marked IsSnapshotable but provides no serialization path: " \
        "either declare `auto SerializeSnapshot(FArchive&, ck::FSnapshotContext&) -> void;` as a member, " \
        "OR make " #_FragmentType_ " a USTRUCT with GENERATED_BODY()."); \
    namespace { \
        struct CK_CONCAT(FCk_SnapshotAutoReg_, __LINE__) { \
            CK_CONCAT(FCk_SnapshotAutoReg_, __LINE__)() { \
                ck::detail::Do_RegisterSnapshotable<_FragmentType_>(TEXT(#_FragmentType_)); \
            } \
        }; \
        static CK_CONCAT(FCk_SnapshotAutoReg_, __LINE__) CK_CONCAT(_AutoReg_, __LINE__){}; \
    }
