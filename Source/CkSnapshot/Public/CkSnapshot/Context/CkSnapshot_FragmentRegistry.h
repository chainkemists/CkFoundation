#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Concepts/CkSnapshot_Concepts.h"

#include "CkSnapshot/Context/CkSnapshot_Context.h"

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp"
#include "CkThirdParty/entt-3.16.0/src/entt/entity/snapshot.hpp"

#include <functional>
#include <type_traits>

class UScriptStruct;
class FArchive;

namespace ck
{
    // Forward-declared so the registry stays on the FragmentRegistry -> (nothing) side of the include graph.
    // The Archive writer/reader includes FragmentRegistry (for DoSerializeSnapshot_OneInstance), so the
    // dependency direction is Archive -> FragmentRegistry. The _Save/_Load closures below reference these
    // types only in their std::function signatures and inside Do_RegisterSnapshotable<T>'s body; the body
    // is a template, so the concrete Archive type only needs to be complete at the CK_REGISTER_SNAPSHOTABLE
    // instantiation site (the fragment's .cpp, which includes the Archive headers).
    class FSnapshotArchive_Writer;
    class FSnapshotArchive_Reader;

    struct CKSNAPSHOT_API FCk_Snapshot_RegisteredFragment
    {
        FString  _DisplayName;
        uint32   _EnttTypeHash = 0;

        std::function<void(entt::basic_snapshot<SnapshotRegistryType>&,          FSnapshotArchive_Writer&)> _Save;
        std::function<void(entt::basic_continuous_loader<SnapshotRegistryType>&, FSnapshotArchive_Reader&)> _Load;

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
        // Unified dispatch for one fragment instance.
        //
        // Const handling: entt's SAVE path (basic_snapshot::get<T>) hands the archive callable a `const T&`
        // because the snapshot is read-only over the registry; the LOAD path (basic_continuous_loader::get<T>)
        // hands a mutable `T&`. Both UScriptStruct::SerializeItem and the Tier-B/C SerializeSnapshot methods are
        // non-const (they read on save, write on load), so we strip const here. On the save path no mutation
        // actually occurs, so this is safe — it matches UE's own serialize-through-const_cast idiom.
        template <typename T>
            requires ck::concepts::FragmentIsSnapshotable<std::remove_const_t<T>>
        auto DoSerializeSnapshot_OneInstance(FArchive& InAr, FSnapshotContext& InCtx, T& InFragment) -> void
        {
            using MutableT = std::remove_const_t<T>;
            auto& MutableFragment = const_cast<MutableT&>(InFragment);

            if constexpr (ck::concepts::FragmentHasCustomSnapshotSerialize<MutableT>)
            {
                // Tier B / C — fragment provides its own serialize body (handles entity-handle remap via InCtx).
                MutableFragment.SerializeSnapshot(InAr, InCtx);
            }
            else
            {
                static_assert(ck::concepts::FragmentIsUStructSnapshotable<MutableT>,
                    "Snapshotable fragment reached dispatch with no serialization path: it must be a USTRUCT "
                    "(StaticStruct) or declare SerializeSnapshot(FArchive&, ck::FSnapshotContext&).");

                // Tier A — plain USTRUCT. The proxy archive already has ArIsSaveGame=true (see the
                // FSnapshotArchive_Writer/Reader ctors), so tagged-property gating drops any field without
                // meta=(SaveGame) automatically.
                MutableT::StaticStruct()->SerializeItem(InAr, &MutableFragment, /*Defaults=*/nullptr);
            }
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

            // entt's basic_snapshot::get<T>(Archive&) writes the storage for fragment type T, routing each
            // instance through the supplied Archive callable (our FSnapshotArchive_Writer, which serializes
            // T via DoSerializeSnapshot_OneInstance and routes entity handles through FSnapshotContext).
            Entry._Save = [](entt::basic_snapshot<SnapshotRegistryType>& InSnap, FSnapshotArchive_Writer& InWriter) -> void
            {
                InSnap.template get<T>(InWriter);
            };

            Entry._Load = [](entt::basic_continuous_loader<SnapshotRegistryType>& InLoader, FSnapshotArchive_Reader& InReader) -> void
            {
                InLoader.template get<T>(InReader);
            };

            FCk_Snapshot_FragmentRegistry::Get().Register(MoveTemp(Entry));
        }
    }
}

// CK_REGISTER_SNAPSHOTABLE(T) -- one line in T's *_Fragment.cpp.
//
// Uses _FragmentType_ (not __LINE__) in the generated struct name to stay unique across unity-build TUs
// where multiple .cpp files may have registrations at the same line number.
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
        struct CK_CONCAT(FCk_SnapshotAutoReg_, _FragmentType_) { \
            CK_CONCAT(FCk_SnapshotAutoReg_, _FragmentType_)() { \
                ck::detail::Do_RegisterSnapshotable<_FragmentType_>(TEXT(#_FragmentType_)); \
            } \
        }; \
        static CK_CONCAT(FCk_SnapshotAutoReg_, _FragmentType_) CK_CONCAT(_AutoReg_, _FragmentType_){}; \
    }
