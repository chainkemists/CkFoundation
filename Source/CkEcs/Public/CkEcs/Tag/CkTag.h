#pragma once

#include <type_traits>

#include "CkEcs/Snapshot/CkSnapshot_TagRegistry.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedTag>
    struct TTag { };
}

// CK_DEFINE_ECS_TAG: defines an empty ECS tag AND auto-registers it as snapshotable (opt-OUT model — every tag
// round-trips by default; the central lifecycle-strip removes markers that must not persist). The registrar is a
// `static inline` variable emitted AFTER the (now-complete) struct — `static inline` is legal at BOTH namespace and
// class scope (a plain `inline` variable is rejected inside a class body, C7524; tags like the signal's nested
// FTag_PayloadInFlight live in class bodies). `[[maybe_unused]]` silences the unused-variable diagnostic; the
// side-effecting initializer is never elided. FCk_Snapshot_TagRegistry::Register dedupes by type-hash, so per-TU /
// per-DLL re-registration is idempotent. Register_SnapshotableTag uses only entt::type_hash + a captureless lambda
// (storage<T> deferred to restore) → safe at static-init, and T is complete here so the lambda instantiates cleanly.
#define CK_DEFINE_ECS_TAG(_TagName_)\
    struct _TagName_ : public ck::TTag<_TagName_> { };\
    static_assert(std::is_empty_v<_TagName_>, "Tags must not carry any data");\
    [[maybe_unused]] static inline const bool _TagAutoReg_##_TagName_ = ::ck::detail::Register_SnapshotableTag<_TagName_>()

#define CK_DEFINE_ECS_TAG_COUNTED(_TagName_)\
    struct _TagName_ : public ck::FTag_CountedTag { };\
    static_assert(sizeof(_TagName_) == sizeof(int32), "Counted Tags must NOT carry any additional data")

// --------------------------------------------------------------------------------------------------------------------