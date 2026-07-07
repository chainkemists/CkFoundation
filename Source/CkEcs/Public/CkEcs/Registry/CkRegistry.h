#pragma once

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry_Handle.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkMemory/Allocator/CkMemoryAllocator.h"

#include "entt/entity/registry.hpp"
#include "entt/core/type_info.hpp"

#include "CkRegistry.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Registry;
struct FCk_Handle_ReadOnly;
class UCk_Utils_EntityTag_UE;
class UCk_Utils_DynamicFragment_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityLifetime_DestroyEntity;
    class FProcessor_ScriptQueryHosted;

    template <typename, typename, typename...>
    class TParallelProcessor;
}

namespace ck
{
    // Context-key type for the registry's transient entity. Stored in
    // entt::registry::ctx() so any FCk_Registry view bound to the same slot
    // sees the same transient entity, regardless of how the view was
    // constructed. Pre-#6 this lived as a per-view _TransientEntity field
    // which made *Handle return a view with no transient entity (footgun);
    // moving to ctx makes the registry the single source of truth.
    struct FCtx_TransientEntity
    {
        FCk_Entity Entity;
    };
}

namespace ck
{
    // this is equivalent to entt::exclude for use with FRegistry::TView<...>
    // usage: Registry.View<CompA, CompB, TExclude<CompC>>().ForEach(...)
    template <typename... T_Args>
    struct TExclude { using ValueType = entt::type_list<T_Args...>; };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FTag_CountedTag
    {
        friend struct FCk_Registry;

    public:
        CK_GENERATED_BODY(FTag_CountedTag);

    private:
        int32 _Count = 0;

    public:
        CK_PROPERTY_GET(_Count);
    };
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Registry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Registry);
    CK_ENABLE_CUSTOM_FORMATTER(FCk_Registry);

public:
    friend class UCk_Utils_EntityLifetime_UE;
    friend class ck::FProcessor_EntityLifetime_DestroyEntity;

    friend struct FCk_Handle;
    friend struct FCk_Handle_ReadOnly;

    template <typename, typename, typename...>
    friend class ck::TParallelProcessor;

public:
    using EntityType = FCk_Entity;

    // The single source of truth for the entt registry type now lives in the
    // slot-table namespace; alias it here for backwards-compatibility with
    // existing callers that reference FCk_Registry::InternalRegistryType.
    using InternalRegistryType = ck::registry_table::EnttRegistryType;

public:
    template <typename T_RegistryType, typename ... T_Fragments>
    class TView
    {
    public:
        template <typename T>
        // ReSharper disable once CppInconsistentNaming
        struct TIsEmpty { static constexpr auto value = std::is_empty_v<T>; };

        template <typename T>
        // ReSharper disable once CppInconsistentNaming
        struct TIsEmpty<ck::TExclude<T>>{ static constexpr auto value = std::is_empty_v<T>; };

        template <typename... T_Args>
        struct TTypeOnly { using TypeList = entt::type_list<T_Args...>; };

        template <typename... T_Args>
        struct TTypeOnly<ck::TExclude<T_Args>...> { using TypeList = entt::type_list<T_Args...>; };

        template <typename... T_Args>
        using TypeOnly_T = entt::type_list_cat_t<typename TTypeOnly<T_Args>::TypeList...>;

        template <typename T>
        struct TIsExcluded : std::false_type { };

        template <typename T>
        struct TIsExcluded<ck::TExclude<T>> : std::true_type { };

        template <typename... T_Args>
        using ExcludesStripped = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value, entt::type_list<>, TypeOnly_T<T_Args>>...>;

        template <typename... T_Args>
        using ExcludesOnly = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value, TypeOnly_T<T_Args>, entt::type_list<>>...>;

        template <typename... T_Args>
        using FragmentsOnly = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value || TIsEmpty<T_Args>::value, entt::type_list<>, TypeOnly_T<T_Args>>...>;

    public:
        using RegistryType = T_RegistryType;
        using FragmentsAndTags = ExcludesStripped<T_Fragments...>;
        using OnlyExcludes = ExcludesOnly<T_Fragments...>;
        using OnlyFragments = FragmentsOnly<T_Fragments...>;

    public:
        explicit TView(RegistryType& InRegistry)
            : _Registry(InRegistry)
        {
        }

        template <typename T_Func>
        auto ForEach(T_Func InFunc)
        {
            DoForEach(InFunc, FragmentsAndTags{}, OnlyExcludes{}, OnlyFragments{});
        }

    private:
        template <typename T_Func, typename... T_FragmentsAndTags, typename... T_OnlyExcludes, typename... T_OnlyFragments>
        auto DoForEach(T_Func InFunc, entt::type_list<T_FragmentsAndTags...>, entt::type_list<T_OnlyExcludes...>, entt::type_list<T_OnlyFragments...>)
        {
            _Registry.template view<T_FragmentsAndTags...>(entt::exclude<T_OnlyExcludes...>).each(
            [InFunc](const EntityType::IdType InEntityId, T_OnlyFragments&... InFragments)
            {
                const auto TypeSafeEntity = FCk_Entity{InEntityId};
                InFunc(TypeSafeEntity, InFragments...);
            });
        }

    private:
        RegistryType& _Registry;
    };

    template <typename... T_Fragments>
    using RegistryViewType = TView<InternalRegistryType, T_Fragments...>;

    template <typename... T_Fragments>
    using ConstRegistryViewType = TView<const InternalRegistryType, T_Fragments...>;

public:
    // FCk_Registry is now a non-owning view. Trivially copyable / movable.
    // Default-constructed views resolve to nullptr.
    FCk_Registry() = default;

    explicit FCk_Registry(FCk_RegistryHandle InHandle)
        : _RegistryHandle(InHandle)
    {}

    CK_PROPERTY_GET(_RegistryHandle);

    // Transient entity is stored in the underlying registry's ctx (see
    // ck::FCtx_TransientEntity). Silent for unset/stale handles — returns a
    // default FCk_Entity rather than firing ensure, because a view that hasn't
    // been bound legitimately has no transient. The owner pushes the entity
    // into ctx via SetContext<ck::FCtx_TransientEntity>(...) on Initialize.
    auto Get_TransientEntity() const -> EntityType;

public:
    template <typename T_Fragment, typename T_Compare>
    auto Sort(T_Compare InCompare) -> void;

    template <typename T_FragmentType, typename T_Func>
    auto Try_Transform(EntityType InEntity, T_Func InFunc) -> void;

private:
    template <typename T_FragmentType, typename... T_Args>
    auto Add(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename... T_Args>
    auto AddOrGet(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename... T_Args>
    auto Replace(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename... T_Args>
    auto AddOrReplace(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_Fragment>
    auto Remove(EntityType InEntity) -> void;

    template <typename T_Fragment>
    auto Try_Remove(EntityType InEntity) -> bool;

    template <typename... T_Fragments>
    auto Clear() -> void;

public:
    template <typename... T_Fragments>
    auto View() -> RegistryViewType<T_Fragments...>;

    template <typename... T_Fragments>
    auto View() const -> ConstRegistryViewType<T_Fragments...>;

    template <typename T_Fragment>
    auto Has_AnyEntityWith() const -> bool;

    // Per-fragment-type version counter used by the scheduler's pump pass to short-circuit
    // dirty-marker checks when nothing has changed since the last read. The counter is
    // incremented by every Add/Replace/AddOrReplace/Remove/Try_Remove/Clear for that type.
    // Storage lives in the slot table keyed by the registry handle so all FCk_Registry views
    // resolved from the same handle observe the same versions.
    // Returns 0 for any hash that has never been mutated (or for an unset/stale handle).
    auto Get_DirtyMarkerVersion(uint32 InFragmentTypeHash) const -> uint64;

    // Runtime-hash counterpart of DoBumpDirtyMarkerVersion<T> for storages the typed mutation
    // paths do not mediate (dynamic script-struct fragments). The hash domain is the caller's —
    // it must match what the consumer registered with the scheduler (see
    // UCk_Utils_DynamicFragment_UE::Get_DirtyMarkerHash for the CkDynamic pairing).
    auto BumpDirtyMarkerVersion(uint32 InFragmentTypeHash) -> void;

    template <typename T_Context, typename... T_Args>
    auto SetContext(T_Args&&... InArgs) -> T_Context&;

    template <typename T_Context>
    auto GetContext() const -> const T_Context&;

    template <typename T_Context>
    auto TryGetContext() const -> const T_Context*;

private:
    // Increments the version counter for T_Fragment in the slot-table side-channel
    // keyed by _RegistryHandle. Called from every mutation path (Add/Replace/
    // AddOrReplace/Remove/Try_Remove/Clear) so the scheduler can detect changes
    // without scanning the storage.
    template <typename T_Fragment>
    auto DoBumpDirtyMarkerVersion() -> void;

    template <typename T_Fragment>
    auto Has(EntityType InEntity) const -> bool;

    template <typename... T_Fragment>
    auto Has_Any(EntityType InEntity) const -> bool;

    template <typename... T_Fragment>
    auto Has_All(EntityType InEntity) const -> bool;

    template <typename T_Fragment>
    auto Get(EntityType InEntity) -> T_Fragment&;

    template <typename T_Fragment>
    auto Get(EntityType InEntity) const -> const T_Fragment&;

private:
    auto CreateEntity() -> EntityType;
    auto CreateEntity(EntityType InEntityHint) -> EntityType;
    auto DestroyEntity(EntityType InEntity) -> void;
    auto DestroyEntities(const TArray<EntityType>& InEntities) -> void;

public:
    auto IsValid(EntityType InEntity) const -> bool;
    auto Orphan(EntityType InEntity) const -> bool;
    auto Get_ValidEntity(EntityType::IdType InEntity) const -> EntityType;

private:
    // TODO: exposing the storage like this is temporary - see branch feature/registry-handle-storage-support for what we really want to do
    friend UCk_Utils_EntityTag_UE;
    friend UCk_Utils_DynamicFragment_UE;
    friend class ck::FProcessor_ScriptQueryHosted;   // typed script-processor join needs the same storage access

    auto Storage()
    {
        return Resolve()->storage();
    }

    template <typename T_Fragment>
    auto&& Storage(entt::id_type InHash)
    {
        return Resolve()->storage<T_Fragment>(InHash);
    }

private:
    // Resolve the underlying entt registry on demand from the slot table. Returns
    // nullptr for unset or stale handles; strict variant fires a non-shipping
    // ensure when stale (programmer-error path), the silent variant is unused
    // here because every FCk_Registry method body either guards with the strict
    // resolve or already implicitly assumed validity (e.g. processor bodies).
    auto Resolve() -> ck::registry_table::EnttRegistryType*
    {
        return ck::registry_table::Resolve(_RegistryHandle);
    }

    auto Resolve() const -> const ck::registry_table::EnttRegistryType*
    {
        return ck::registry_table::Resolve(_RegistryHandle);
    }

public:
    friend auto CKECS_API GetTypeHash(const ThisType& InRegistry) -> uint32;

private:
    UPROPERTY()
    FCk_RegistryHandle _RegistryHandle;
};

// --------------------------------------------------------------------------------------------------------------------

auto CKECS_API GetTypeHash(const FCk_Registry& InRegistry) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Registry, [](const FCk_Registry& InObj)
{
    // Escape the literal braces: {{ -> '{', }} -> '}', and {} are the two positional args. The former
    // TEXT("{slot={},gen={}}") made fmt parse "{slot=...}" as a NAMED field 'slot' it couldn't resolve,
    // throwing fmt::format_error — which crashed every ensure that formats a registry (e.g. the tombstone-
    // handle ensures fired during a CkSnapshot load).
    return ck::Format
    (
        TEXT("{{slot={},gen={}}}"),
        InObj.Get_RegistryHandle().SlotIndex,
        InObj.Get_RegistryHandle().Generation
    );
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Registry, IsValid_Policy_Default, [=](const FCk_Registry& InRegistry)
{
    return ck::registry_table::TryResolve(InRegistry.Get_RegistryHandle()) != nullptr;
});

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment>
auto
    FCk_Registry::
    Has_AnyEntityWith() const
    -> bool
{
    const auto* Reg = Resolve();
    if (Reg == nullptr) { return false; }

    const auto* Storage = Reg->template storage<T_Fragment>();
    if (Storage == nullptr)
    { return false; }
    return NOT Storage->empty();
}

template <typename T_Fragment>
auto
    FCk_Registry::
    DoBumpDirtyMarkerVersion()
    -> void
{
    const auto Hash = static_cast<uint32>(entt::type_hash<T_Fragment>::value());
    ck::registry_table::BumpDirtyMarkerVersion(_RegistryHandle, Hash);
}

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Context, typename... T_Args>
auto
    FCk_Registry::
    SetContext(
        T_Args&&... InArgs)
    -> T_Context&
{
    return Resolve()->ctx().emplace<T_Context>(std::forward<T_Args>(InArgs)...);
}

template <typename T_Context>
auto
    FCk_Registry::
    GetContext() const
    -> const T_Context&
{
    return Resolve()->ctx().get<const T_Context>();
}

template <typename T_Context>
auto
    FCk_Registry::
    TryGetContext() const
    -> const T_Context*
{
    // Silent path — uses TryResolve so an unset/stale FCk_Registry returns
    // nullptr without firing ensure. "Try" in the name; callers (including
    // Get_TransientEntity) rely on this being non-noisy.
    const auto* Reg = ck::registry_table::TryResolve(_RegistryHandle);
    if (Reg == nullptr) { return nullptr; }
    return Reg->ctx().find<const T_Context>();
}

inline auto
    FCk_Registry::
    Get_TransientEntity() const
    -> EntityType
{
    if (const auto* Found = TryGetContext<ck::FCtx_TransientEntity>())
    { return Found->Entity; }
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Registry::
    Add(
        EntityType InEntity,
        T_Args&&... InArgs)
    -> T_FragmentType&
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::Add"));
#endif

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    if constexpr (std::is_empty_v<T_FragmentType>)
    {
        static_assert(std::is_base_of_v<ck::TTag<T_FragmentType>, T_FragmentType>, "Tags must derive from ck::TTag (see helper macro)");
        static T_FragmentType Empty_Tag;

        CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity) == false,
                TEXT("Tag [{}] already exists in Entity [{}]."),
                ck::TypeToString<T_FragmentType>, InEntity)
        { return Empty_Tag; }

        Resolve()->emplace<T_FragmentType>(InEntity.Get_ID());
        DoBumpDirtyMarkerVersion<T_FragmentType>();
        return Empty_Tag;
    }
    else
    {
        if constexpr (std::is_base_of_v<ck::FTag_CountedTag, T_FragmentType>)
        {
            auto& Fragment = Has<T_FragmentType>(InEntity) ?
                Get<T_FragmentType>(InEntity) :
                Resolve()->emplace<T_FragmentType>(InEntity.Get_ID());

            ++Fragment._Count;
            DoBumpDirtyMarkerVersion<T_FragmentType>();
            return Fragment;
        }
        else
        {
            CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity) == false && (std::is_base_of_v<ck::FTag_CountedTag, T_FragmentType>) == false,
                TEXT("Fragment [{}] already exists in Entity [{}]."),
                ck::TypeToString<T_FragmentType>, InEntity)
            {
                static T_FragmentType Invalid_Fragment;
                return Invalid_Fragment;
            }

            auto& Fragment = Resolve()->emplace<T_FragmentType>(InEntity.Get_ID(), std::forward<T_Args>(InArgs)...);
            DoBumpDirtyMarkerVersion<T_FragmentType>();
            return Fragment;
        }
    }
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Registry::
    AddOrGet(
        EntityType InEntity,
        T_Args&&... InArgs)
    -> T_FragmentType&
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::AddOrGet"));
#endif

    if (Has<T_FragmentType>(InEntity))
    {
        // Callers of AddOrGet intend to mutate the fragment in-place (e.g. appending to a
        // request TArray). In-place mutations on an already-present fragment don't otherwise
        // touch the registry, so the scheduler's dirty-marker version would not advance and
        // pump short-circuit would gate out a legitimately dirty processor. Bumping here
        // ensures request-queueing patterns like DoAddRequest propagate the same frame.
        DoBumpDirtyMarkerVersion<T_FragmentType>();
        return Get<T_FragmentType>(InEntity);
    }

    return Add<T_FragmentType>(InEntity, std::forward<T_Args>(InArgs)...);
}

template <typename T_FragmentType, typename T_Func>
auto
    FCk_Registry::
    Try_Transform(
        EntityType InEntity,
        T_Func InFunc)
    -> void
{
    if (NOT Has<T_FragmentType>(InEntity))
    { return; }

    InFunc(Get<T_FragmentType>(InEntity));
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Registry::
    Replace(
        EntityType InEntity,
        T_Args&&... InArgs)
    -> T_FragmentType&
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::Replace"));
#endif

    static_assert(std::is_empty_v<T_FragmentType> == false, "You can only Replace Fragments with data.");

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Replace Fragment"), InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity),
        TEXT("Unable to Replace Fragment. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
        ck::TypeToString<T_FragmentType>, InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    auto& Fragment = Resolve()->get<T_FragmentType>(InEntity.Get_ID());
    Fragment = T_FragmentType{ std::forward<T_Args>(InArgs)... };

    DoBumpDirtyMarkerVersion<T_FragmentType>();
    return Fragment;
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Registry::
    AddOrReplace(
        EntityType InEntity,
        T_Args&&... InArgs)
    -> T_FragmentType&
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::AddOrReplace"));
#endif

    static_assert(std::is_empty_v<T_FragmentType> == false, "You can only AddOrReplace Fragments with data.");

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to AddOrReplace Fragment"), InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    if (Has<T_FragmentType>(InEntity))
    { Remove<T_FragmentType>(InEntity); }

    return Add<T_FragmentType>(std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Remove(
        EntityType InEntity)
    -> void
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::Remove"));
#endif

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Remove Fragment/Tag."), InEntity)
    { return; }

    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
        TEXT("Unable to Remove Fragment/Tag. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
        ck::TypeToString<T_Fragment>, InEntity)
    { return; }

    if constexpr (std::is_base_of_v<ck::FTag_CountedTag, T_Fragment>)
    {
        auto& Fragment = Get<T_Fragment>(InEntity);
        --Fragment._Count;

        DoBumpDirtyMarkerVersion<T_Fragment>();

        if (Fragment._Count > 0)
        { return; }
    }

    Resolve()->remove<T_Fragment>(InEntity.Get_ID());
    DoBumpDirtyMarkerVersion<T_Fragment>();
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Try_Remove(
        EntityType InEntity)
    -> bool
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::Try_Remove"));
#endif

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to TryRemove Fragment/Tag."), InEntity)
    { return false; }

    const auto RemovedAny = Resolve()->remove<T_Fragment>(InEntity.Get_ID()) > 0;
    if (RemovedAny)
    {
        DoBumpDirtyMarkerVersion<T_Fragment>();
    }
    return RemovedAny;
}

template <typename ... T_Fragments>
auto
    FCk_Registry::
    Clear()
    -> void
{
    // entt's zero-arg registry::clear() wipes EVERY pool and destroys ALL entities — the per-type
    // fold below would silently no-op instead. No caller wants either surprise from an empty pack.
    static_assert(sizeof...(T_Fragments) > 0, "Clear requires at least one explicit fragment type");

#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::Clear"));
#endif

    // Per-type: skip pools with no packed entries (live OR tombstoned). End-of-frame cleanup
    // processors call Clear unconditionally every frame, and bumping the version of a pool that
    // provably had nothing to remove would re-dirty the scheduler's persistent pump short-circuit
    // for that marker every frame. A tombstone-only pool still clears (and bumps one last time) so
    // its packed array — and Has_AnyEntityWith's view of it — resets to truly empty.
    ([&]
    {
        if (Has_AnyEntityWith<T_Fragments>())
        {
            Resolve()->clear<T_Fragments>();
            DoBumpDirtyMarkerVersion<T_Fragments>();
        }
    }(), ...);
}

template <typename... T_Fragments>
auto
    FCk_Registry::
    View()
    -> RegistryViewType<T_Fragments...>
{
    return TView<InternalRegistryType, T_Fragments...>{*Resolve()};
}

template <typename... T_Fragments>
auto
    FCk_Registry::
    View() const
    -> ConstRegistryViewType<T_Fragments...>
{
    return TView<const InternalRegistryType, T_Fragments...>{*Resolve()};
}

template <typename T_Fragment, typename T_Compare>
auto
    FCk_Registry::
    Sort(
        T_Compare InCompare)
    -> void
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::Sort"));
#endif

    Resolve()->sort<T_Fragment>(InCompare);
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Has(
        EntityType InEntity) const
    -> bool
{
    return Resolve()->any_of<T_Fragment>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto
    FCk_Registry::
    Has_Any(
        EntityType InEntity) const
    -> bool
{
    return Resolve()->any_of<T_Fragment...>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto
    FCk_Registry::
    Has_All(
        EntityType InEntity) const
    -> bool
{
    return Resolve()->all_of<T_Fragment...>(InEntity.Get_ID());
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Get(
        EntityType InEntity)
    -> T_Fragment&
{
    static T_Fragment Invalid_Fragment;

    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
         TEXT("Unable to Get Fragment. Fragment [{}] does NOT exist in Entity [{}]."),
         ck::TypeToString<T_Fragment>, InEntity)
    { return Invalid_Fragment; }

    if constexpr (std::is_empty_v<T_Fragment>)
    {
        static T_Fragment Empty_Tag;
        return Empty_Tag;
    }
    else
    {
        return Resolve()->get<T_Fragment>(InEntity.Get_ID());
    }
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Get(
        EntityType InEntity) const
    -> const T_Fragment&
{
    static T_Fragment Invalid_Fragment;

    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
        TEXT("Unable to Get Fragment. Fragment [{}] does NOT exist in Entity [{}]."),
        ck::TypeToString<T_Fragment>, InEntity)
    { return Invalid_Fragment; }

    if constexpr (std::is_empty_v<T_Fragment>)
    {
        static T_Fragment Empty_Tag;
        return Empty_Tag;
    }
    else
    {
        return Resolve()->get<T_Fragment>(InEntity.Get_ID());
    }
}

// --------------------------------------------------------------------------------------------------------------------
