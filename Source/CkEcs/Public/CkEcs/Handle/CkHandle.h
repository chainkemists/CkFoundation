#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Policy/CkPolicy.h"
#include "CkEcs/Entity/CkEntity.h"
#include "CkCore/AngelScript/CkAngelscriptDebugger.h"
#include "CkEcs/Handle/CkHandle_Debugging.h"
#include "CkEcs/Registry/CkRegistry_Handle.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include "Iris/Serialization/NetSerializer.h"
#include "Iris/Serialization/NetSerializerConfig.h"

#include "CkHandle.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FArchive;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Declared here rather than in CkNet_Fragment: handle debugging needs them, and that would be circular
    CK_DEFINE_ECS_TAG(FTag_NetMode_IsHost);
    CK_DEFINE_ECS_TAG(FTag_NetMode_IsClient);
}

namespace ck
{
    // Declared here rather than in CkEntityLifetime_Fragment: handle debugging needs them, and that would be circular
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Initiate);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_EndPlay);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Teardown);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Await);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Finalize);
    CK_DEFINE_ECS_TAG(FTag_EntityJustCreated);

    template <typename T>
    auto
    Get_LifetimeTagString()
    {
        // Reverse phase order: an Entity keeps every lifetime tag it was ever given, so the latest must win

        if constexpr (std::is_same_v<T, FTag_DestroyEntity_Finalize>)
        { return TEXT("D_Final"); }

        if constexpr (std::is_same_v<T, FTag_DestroyEntity_Await>)
        { return TEXT("D_Await"); }

        if constexpr (std::is_same_v<T, FTag_DestroyEntity_Teardown>)
        { return TEXT("D_Teardown"); }

        if constexpr (std::is_same_v<T, FTag_DestroyEntity_EndPlay>)
        { return TEXT("D_EndPlay"); }

        if constexpr (std::is_same_v<T, FTag_DestroyEntity_Initiate>)
        { return TEXT("D_Init"); }

        if constexpr (std::is_same_v<T, FTag_EntityJustCreated>)
        { return TEXT("E_New"); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

template<typename Type>
struct entt::component_traits<Type> {
    // ReSharper disable once CppInconsistentNaming
    static constexpr bool in_place_delete = true;
    // ReSharper disable once CppInconsistentNaming
    static constexpr std::size_t page_size = !std::is_empty_v<Type> * ENTT_PACKED_PAGE;
};

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING

// --------------------------------------------------------------------------------------------------------------------

struct DEBUG_NAME
{
    friend class UCk_Utils_Handle_UE;
    friend struct FEntity_FragmentMapper;
public:
    CK_GENERATED_BODY(DEBUG_NAME);

private:
    FName _Name;
    TArray<FName> _PreviousNames;

private:
    auto
    DoSet_DebugName(FName InDebugName, ECk_Override InOverride = ECk_Override::Override) -> void;

public:
    CK_PROPERTY_GET(_Name);

    CK_DEFINE_CONSTRUCTORS(DEBUG_NAME, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

template <>
struct TCk_DebugWrapper<DEBUG_NAME> : public FCk_DebugWrapper
{
public:
    explicit TCk_DebugWrapper(const DEBUG_NAME* InPtr);

    auto GetHash() const -> IdType override;
    auto SetFragmentPointer(const void* InFragmentPtr) -> void override;
    auto Get_FragmentName(const FCk_Handle& InHandle) const -> FName override;

    auto operator==(const TCk_DebugWrapper& InOther) const -> bool;
    auto operator!=(const TCk_DebugWrapper& InOther) const -> bool;

private:
    const DEBUG_NAME* _Fragment = nullptr;
};

#else

#endif

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace UE::Net { struct FCk_HandleNetSerializer; }

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(NoImplicitConversion, HasNativeMake, HasNativeBreak="/Script/CkEcs.Ck_Utils_Handle_UE:Break_Handle"))
struct CKECS_API FCk_Handle
{
    GENERATED_BODY()

    friend class UCk_Utils_EntityReplicationDriver_UE;
    friend class UCk_Fragment_EntityReplicationDriver_Rep;
    friend struct UE::Net::FCk_HandleNetSerializer;

public:
    CK_GENERATED_BODY(FCk_Handle);
    CK_ENABLE_CUSTOM_FORMATTER(FCk_Handle);

public:
    using EntityType = FCk_Entity;
    using RegistryType = FCk_Registry;

public:
    FCk_Handle() = default;

    FCk_Handle(EntityType InEntity, FCk_RegistryHandle InRegistry);

    // this is a special hard-coded function that expects the type-safe handle to have a particular function
    template <typename T_WrappedHandle, class = std::enable_if_t<std::is_base_of_v<struct FCk_Handle_TypeSafe, T_WrappedHandle>>>
    FCk_Handle(const T_WrappedHandle& InTypeSafeHandle);

public:
    auto Swap(ThisType& InOther) -> void;

public:
    template <typename T_WrappedHandle, class = std::enable_if_t<std::is_base_of_v<struct FCk_Handle_TypeSafe, T_WrappedHandle>>>
    auto operator==(const T_WrappedHandle& InOther) const -> bool;

    template <typename T_WrappedHandle, class = std::enable_if_t<std::is_base_of_v<struct FCk_Handle_TypeSafe, T_WrappedHandle>>>
    auto operator!=(const T_WrappedHandle& InOther) const -> bool;

    auto operator<(const ThisType& InOther) const -> bool;
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

public:
    template <typename T_Fragment, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> T_Fragment&;

    template <typename T_Fragment, typename T_ValidationPolicy, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> T_Fragment&;

    template <typename T_Fragment, typename... T_Args>
    auto AddOrGet(T_Args&&... InArgs) -> T_Fragment&;

    template <typename T_Fragment, typename T_ValidationPolicy, typename... T_Args>
    auto AddOrGet(T_Args&&... InArgs) -> T_Fragment&;

    template <typename T_Fragment, typename T_Func>
    auto Try_Transform(T_Func InFunc) -> void;

    template <typename T_Fragment, typename... T_Args>
    auto Replace(T_Args&&... InArgs) -> T_Fragment&;

    template <typename T_Fragment, typename... T_Args>
    auto AddOrReplace(T_Args&&... InArgs) -> T_Fragment&;

    // The 2-parameter forms take EITHER policy — a ck::IsValid_Policy or a removal policy
    // (ck::policy::ForceErase) — and default the other, so neither has to be spelled to reach
    // the other. T_RemovalPolicy is honored for COUNTED tags only; see FCk_Registry::Remove.
    template <typename T_Fragment>
    auto Remove() -> void;

    template <typename T_Fragment, typename T_Policy>
    auto Remove() -> void;

    template <typename T_Fragment, typename T_ValidationPolicy, typename T_RemovalPolicy>
    auto Remove() -> void;

    template <typename T_Fragment, typename T_FragmentFunc>
    auto CopyAndRemove(T_Fragment FragmentToCopyAndRemove, T_FragmentFunc InFunc) -> void;

    template <typename T_Fragment>
    auto Try_Remove() -> bool;

    template <typename T_Fragment, typename T_Policy>
    auto Try_Remove() -> bool;

    template <typename T_Fragment, typename T_ValidationPolicy, typename T_RemovalPolicy>
    auto Try_Remove() -> bool;

    template <typename... T_Fragments>
    auto Clear() -> void;

    // See FCk_Registry::Clear_Unconditional — kernel frame-boundary markers only.
    template <typename... T_Fragments>
    auto Clear_Unconditional() -> void;

    template <typename... T_Fragment>
    auto View() -> FCk_Registry::RegistryViewType<T_Fragment...>;

    template <typename... T_Fragment>
    auto View() const -> FCk_Registry::ConstRegistryViewType<T_Fragment...>;

public:
    template <typename T_Fragment>
    auto Has() const -> bool;

    template <typename... T_Fragment>
    auto Has_Any() const -> bool;

    template <typename... T_Fragment>
    auto Has_All() const -> bool;

    // Registry-wide queries (NOT about this handle's entity) — forwarded to the handle's registry.
    // Tombstone-aware: true only if some LIVE entity in this handle's world has the fragment
    // (see FCk_Registry::Has_AnyLiveEntityWith for the tombstone rationale and cost note).
    template <typename T_Fragment>
    auto Has_AnyLiveEntityWith() const -> bool;

    // Same, but skipping entities that also carry any of T_Exclude (e.g. the pending-kill tags).
    template <typename T_Fragment, typename... T_Exclude>
    auto Has_AnyLiveEntityWith_Excluding() const -> bool;

    template <typename T_Fragment>
    auto Get() -> T_Fragment&;

    template <typename T_Fragment>
    auto Get() const -> const T_Fragment&;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() -> T_Fragment&;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() const -> const T_Fragment&;

public:
    // Returns FCk_Registry BY VALUE (a trivially copyable non-owning view) — do NOT bind
    // the result to a reference, it is a temporary.
    auto operator*()       -> FCk_Registry;
    auto operator*() const -> const FCk_Registry;

    auto operator->()       -> FCk_Registry;
    auto operator->() const -> const FCk_Registry;

public:
    auto IsValid(ck::IsValid_Policy_Default) const -> bool;
    auto IsValid(ck::IsValid_Policy_IncludePendingKill) const -> bool;
    auto IsRegistryValid() const -> bool;
    auto ToString() const -> FString;

    auto Orphan() const -> bool;
    auto Get_ValidHandle(EntityType::IdType InEntity) const -> ThisType;

    // Non-owning, returned by value — do NOT bind to a reference. Carries no transient
    // entity; for that use UCk_EcsWorld_Subsystem_UE::Get_Registry() instead.
    auto Get_RegistryView()       -> FCk_Registry;
    auto Get_RegistryView() const -> const FCk_Registry;

    // needed for AngelScript implicit conversion
    auto
    ConvertToHandle() const -> FCk_Handle;

    // this is only for Angelscript debugging
    auto
    DoFireEnsure() const -> void;

public:
    auto
    Get_DebugName() const -> FName;

public:
    auto
    NetSerialize(
        FArchive& Ar,
        class UPackageMap* Map,
        bool& bOutSuccess) -> bool;

private:
    template <typename T_Fragment>
    requires(std::is_empty_v<T_Fragment>)
    auto DoClear(bool InSkipQuarantined) -> void;

    template <typename T_Fragment>
    auto DoClear(bool InSkipQuarantined) -> void;

    // Mutates registry debug state and is NOT thread-safe — no-ops off the game thread and
    // inside a parallel region; parallel processors use FCk_Handle_ReadOnly.
    auto DoUpdate_FragmentDebugInfo_Blueprints() -> void;

    template <typename T_Fragment>
    auto DoAdd_FragmentDebugInfo() -> void;

    template <typename T_Fragment>
    auto DoRemove_FragmentDebugInfo() -> void;

protected:
    // Transient: entity-id + registry slot are session-specific RUNTIME values and must never persist as raw
    // bytes. CkSnapshot persists a handle only by REMAPPING the id through FSnapshotContext::Snapshot_Handle;
    // Transient keeps these fields out of its whole-fragment data pass. See CkEcs/CLAUDE.md.
    UPROPERTY(BlueprintReadOnly, NotReplicated, Transient)
    FCk_Entity _Entity;

    // Trivially copyable POD (slot-index + generation), resolved to the live entt::registry on demand.
    // Transient (see _Entity): re-homed onto the live registry by Snapshot_Handle on load, never persisted raw.
    UPROPERTY(Transient)
    FCk_RegistryHandle _RegistryHandle;

private:
    // Transient (see _Entity): a weak ptr to a runtime replication-driver object -- session-specific, never persisted.
    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_Ecs_ReplicatedObject_UE> _ReplicationDriver;

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    const struct FEntity_FragmentMapper* _Mapper = nullptr;
#endif

public:
    CK_PROPERTY(_Entity);

    // Re-home onto a different registry slot: after a cross-world restore a deserialized handle's
    // id is remapped but its slot still points at the saving world's, so Snapshot_Handle calls this.
    auto Set_Registry(FCk_RegistryHandle InRegistryHandle) -> void;

#if WITH_EDITORONLY_DATA
private:
    UPROPERTY(NotReplicated, Transient) // needs to be a UPROPERTY so that it shows up when debugging Blueprints
    TWeakObjectPtr<class UCk_Handle_FragmentsDebug> _Fragments = nullptr;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
// Legacy

template<>
struct TStructOpsTypeTraits<FCk_Handle> : public TStructOpsTypeTraitsBase2<FCk_Handle>
{
    enum
    {
        /*
         * FStructs in Blueprints are compared using CompareScriptStruct through FStructProperty::Identical when Set/Map invokes their Equality function objects in their FindIndex implementations.
         * This is unexpected as the comparison is done using reflection instead of invoking the overloaded operator==.
         * The fix for this is to use the TStructOpsTypeTraits with WithIdenticalViaEquality set to true to force the reflection mechanisms to use the overloaded operator== found in our FStruct (in this case, FCk_Handle)
         */
        WithIdenticalViaEquality = true,
        WithNetSerializer = true
    };
};

// --------------------------------------------------------------------------------------------------------------------
// Iris

USTRUCT()
struct FCk_HandleSerializerConfig : public FNetSerializerConfig
{
    GENERATED_BODY()
};

// ReSharper disable once CppInconsistentNaming
namespace UE::Net
{
    UE_NET_DECLARE_SERIALIZER(FCk_HandleNetSerializer, CKECS_API);
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_IF_HANDLE_IS_PENDING_KILL(_Handle_)\
if (ck::Is_NOT_Valid(_Handle_) && ck::IsValid(_Handle_, ck::IsValid_Policy_IncludePendingKill{}))

// --------------------------------------------------------------------------------------------------------------------

auto CKECS_API GetTypeHash(const FCk_Handle& InHandle) -> uint32;

namespace ck
{
    // Entity and Handle overloads exist in pairs so generic call sites never care which one they hold

    auto CKECS_API
    MakeHandle(
        FCk_Entity InEntity,
        const FCk_Registry& InValidHandle) -> FCk_Handle;

    auto CKECS_API
    MakeHandle(
        FCk_Entity InEntity,
        FCk_Handle InValidHandle) -> FCk_Handle;

    auto CKECS_API
    MakeHandle(
        FCk_Handle InEntity,
        FCk_Handle InValidHandle) -> FCk_Handle;

    auto CKECS_API
    IsValid(
        FCk_Entity InEntity,
        FCk_Handle InValidHandle) -> bool;

    auto CKECS_API
    IsValid(
        const FCk_Handle& InEntity,
        FCk_Handle InValidHandle) -> bool;

    auto CKECS_API
    GetEntity(
        FCk_Entity InEntity) -> FCk_Entity;

    auto CKECS_API
    GetEntity(
        const FCk_Handle& InEntity) -> FCk_Entity;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    struct CKECS_API MatchesEntityHandle
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FCk_Handle _EntityHandle;

    public:
        CK_DEFINE_CONSTRUCTOR(MatchesEntityHandle, _EntityHandle);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API IsValidEntityHandle
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API IsValidEntityHandle_IncludePendingKill
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Handle, IsValid_Policy_Default, [&](const FCk_Handle& InHandle)
{
    return InHandle.IsValid(ck::IsValid_Policy_Default{});
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Handle, IsValid_Policy_IncludePendingKill, [&](const FCk_Handle& InHandle)
{
    return InHandle.IsValid(ck::IsValid_Policy_IncludePendingKill{});
});

// --------------------------------------------------------------------------------------------------------------------

CK_DECLARE_CUSTOM_FORMATTER_WITH_DETAILS(CKECS_API, FCk_Handle);

// --------------------------------------------------------------------------------------------------------------------

template <typename T_WrappedHandle, class>
FCk_Handle::
    FCk_Handle(
        const T_WrappedHandle& InOther)
    : _Entity(InOther._Entity)
    , _RegistryHandle(InOther._RegistryHandle)
    , _ReplicationDriver(InOther._ReplicationDriver)
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    , _Mapper(InOther._Mapper)
#endif
#if WITH_EDITORONLY_DATA
    , _Fragments(InOther._Fragments)
#endif
{
    DoUpdate_FragmentDebugInfo_Blueprints();
}

template <typename T_WrappedHandle, class>
auto
    FCk_Handle::
    operator==(
        const T_WrappedHandle& InOther) const
    -> bool
{
    return InOther == *this;
}

template <typename T_WrappedHandle, class>
auto
    FCk_Handle::
    operator!=(
        const T_WrappedHandle& InOther) const
    -> bool
{
    return InOther != *this;
}

template <typename T_Fragment, typename ... T_Args>
auto
    FCk_Handle::
    Add(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    return Add<T_Fragment, ck::IsValid_Policy_Default>(std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment, typename T_ValidationPolicy, typename ... T_Args>
auto
    FCk_Handle::
    Add(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    static_assert(std::is_base_of_v<ck::IsValid_Policy, T_ValidationPolicy>,
        "T_ValidationPolicy must be a ck::IsValid_Policy");

    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    auto& NewFragment = Get_RegistryView().Add<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

    if constexpr (std::is_base_of_v<ck::FTag_CountedTag, T_Fragment>)
    {
        const auto TagWasAlreadyPresent = NewFragment.Get_Count() > 1;
        if (TagWasAlreadyPresent)
        {
            return NewFragment;
        }
    }

    DoAdd_FragmentDebugInfo<T_Fragment>();
    DoUpdate_FragmentDebugInfo_Blueprints();

    return NewFragment;
}

template <typename T_Fragment, typename T_ValidationPolicy, typename ... T_Args>
auto
    FCk_Handle::
    AddOrGet(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to AddOrGet Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (NOT IsRegistryValid())
            { return TEXT("does NOT have a valid Registry"); }

            auto Reg = Get_RegistryView();
            if (NOT Reg.IsValid(_Entity))
            { return TEXT("does NOT have a valid Entity"); }

            return TEXT("");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    auto& NewOrExistingFragment = [&]() -> T_Fragment&
    {
        const auto AddDebugInfo = NOT Has<T_Fragment>();

        auto& Fragment = Get_RegistryView().AddOrGet<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

        if (AddDebugInfo)
        {
            if constexpr (std::is_base_of_v<ck::FTag_CountedTag, T_Fragment>)
            {
                const auto TagWasAlreadyPresent = Fragment.Get_Count() > 1;
                if (TagWasAlreadyPresent)
                {
                    return Fragment;
                }
            }

            DoAdd_FragmentDebugInfo<T_Fragment>();
            DoUpdate_FragmentDebugInfo_Blueprints();
        }

        return Fragment;
    }();

    return NewOrExistingFragment;
}

template <typename T_Fragment, typename ... T_Args>
auto
    FCk_Handle::
    AddOrGet(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    CK_ENSURE_IF_NOT(IsValid(ck::IsValid_Policy_Default{}),
        TEXT("Unable to AddOrGet Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    auto& NewOrExistingFragment = [&]() -> T_Fragment&
    {
        const auto AddDebugInfo = NOT Has<T_Fragment>();

        auto& Fragment = Get_RegistryView().AddOrGet<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

        if (AddDebugInfo)
        {
            DoAdd_FragmentDebugInfo<T_Fragment>();
            DoUpdate_FragmentDebugInfo_Blueprints();
        }

        return Fragment;
    }();

    return NewOrExistingFragment;
}

template <typename T_Fragment, typename T_Func>
auto
    FCk_Handle::
    Try_Transform(
        T_Func InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(IsValid(ck::IsValid_Policy_Default{}),
        TEXT("Unable to Transform Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    { return; }

    Get_RegistryView().Try_Transform<T_Fragment>(_Entity, InFunc);
}

template <typename T_Fragment, typename ... T_Args>
auto
    FCk_Handle::
    Replace(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    CK_ENSURE_IF_NOT(IsValid(ck::IsValid_Policy_Default{}),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return Get_RegistryView().Replace<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment, typename ... T_Args>
auto
    FCk_Handle::
    AddOrReplace(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    CK_ENSURE_IF_NOT(IsValid(ck::IsValid_Policy_Default{}),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return Get_RegistryView().AddOrReplace<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Remove()
    -> void
{
    Remove<T_Fragment, ck::IsValid_Policy_Default, void>();
}

template <typename T_Fragment, typename T_Policy>
auto
    FCk_Handle::
    Remove()
    -> void
{
    if constexpr (std::is_base_of_v<ck::IsValid_Policy, T_Policy>)
    { Remove<T_Fragment, T_Policy, void>(); }
    else
    { Remove<T_Fragment, ck::IsValid_Policy_Default, T_Policy>(); }
}

template <typename T_Fragment, typename T_ValidationPolicy, typename T_RemovalPolicy>
auto
    FCk_Handle::
    Remove()
    -> void
{
    static_assert(std::is_base_of_v<ck::IsValid_Policy, T_ValidationPolicy>,
        "T_ValidationPolicy must be a ck::IsValid_Policy");

    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    { return; }

    Get_RegistryView().Remove<T_Fragment, T_RemovalPolicy>(_Entity);

    if constexpr (std::is_base_of_v<ck::FTag_CountedTag, T_Fragment>)
    {
        const auto TagIsStillPresent = Has<T_Fragment>();
        if (TagIsStillPresent)
        {
            return;
        }
    }

    DoRemove_FragmentDebugInfo<T_Fragment>();
}

template <typename T_Fragment, typename T_FragmentFunc>
auto
    FCk_Handle::
    CopyAndRemove(
        T_Fragment FragmentToCopyAndRemove,
        T_FragmentFunc InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(IsValid(ck::IsValid_Policy_Default{}),
        TEXT("Unable to CopeAndRemove Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    { return; }

    T_Fragment FragmentCopy;
    ::Swap(FragmentToCopyAndRemove, FragmentCopy);
    Remove<T_Fragment>();

    InFunc(FragmentCopy);
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Try_Remove()
    -> bool
{
    return Try_Remove<T_Fragment, ck::IsValid_Policy_Default, void>();
}

template <typename T_Fragment, typename T_Policy>
auto
    FCk_Handle::
    Try_Remove()
    -> bool
{
    if constexpr (std::is_base_of_v<ck::IsValid_Policy, T_Policy>)
    { return Try_Remove<T_Fragment, T_Policy, void>(); }
    else
    { return Try_Remove<T_Fragment, ck::IsValid_Policy_Default, T_Policy>(); }
}

template <typename T_Fragment, typename T_ValidationPolicy, typename T_RemovalPolicy>
auto
    FCk_Handle::
    Try_Remove()
    -> bool
{
    static_assert(std::is_base_of_v<ck::IsValid_Policy, T_ValidationPolicy>,
        "T_ValidationPolicy must be a ck::IsValid_Policy");

    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (IsRegistryValid())
            {
                auto Reg = Get_RegistryView();
                if (Reg.IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    { return false; }

    const auto Result = Get_RegistryView().Try_Remove<T_Fragment, T_RemovalPolicy>(_Entity);

    if constexpr (std::is_base_of_v<ck::FTag_CountedTag, T_Fragment>)
    {
        const auto TagIsStillPresent = Has<T_Fragment>();
        if (TagIsStillPresent)
        {
            return true;
        }
    }

    DoRemove_FragmentDebugInfo<T_Fragment>();

    return Result;
}

template <typename ... T_Fragments>
auto
    FCk_Handle::
    Clear()
    -> void
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to Clear<...> Fragments. Handle [{}] does NOT have a valid Registry."), *this)
    { return; }

    // The per-fragment pass is what lets Entity debugging clear its debug mapping. It mirrors the registry's
    // quarantine skip: a debugger row saying a fragment is gone while the entity still holds it is exactly
    // the kind of mirror that sends the next reader looking in the wrong place.
    constexpr auto SkipQuarantined = true;
#if CK_DISABLE_ECS_HANDLE_DEBUGGING == 0
    (DoClear<T_Fragments>(SkipQuarantined), ...);
#endif
    Get_RegistryView().Clear<T_Fragments...>();
}

template <typename ... T_Fragments>
auto
    FCk_Handle::
    Clear_Unconditional()
    -> void
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to Clear<...> Fragments. Handle [{}] does NOT have a valid Registry."), *this)
    { return; }

    constexpr auto SkipQuarantined = false;
#if CK_DISABLE_ECS_HANDLE_DEBUGGING == 0
    (DoClear<T_Fragments>(SkipQuarantined), ...);
#endif
    Get_RegistryView().Clear_Unconditional<T_Fragments...>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    View()
    -> FCk_Registry::RegistryViewType<T_Fragment...>
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static RegistryType Invalid_Registry;
        return Invalid_Registry.View<T_Fragment...>();
    }

    return Get_RegistryView().View<T_Fragment...>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    View() const
    -> FCk_Registry::ConstRegistryViewType<T_Fragment...>
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static RegistryType Invalid_Registry;
        return Invalid_Registry.View<T_Fragment...>();
    }

    return Get_RegistryView().View<T_Fragment...>();
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Has() const
    -> bool
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to perform Has query with Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this)
    { return {}; }

    return Get_RegistryView().Has<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    Has_Any() const
    -> bool
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to perform Has_Any query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return Get_RegistryView().Has_Any<T_Fragment...>(_Entity);
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    Has_All() const
    -> bool
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to perform Has_All query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return Get_RegistryView().Has_All<T_Fragment...>(_Entity);
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Has_AnyLiveEntityWith() const
    -> bool
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to perform Has_AnyLiveEntityWith query with Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this)
    { return {}; }

    return Get_RegistryView().Has_AnyLiveEntityWith<T_Fragment>();
}

template <typename T_Fragment, typename... T_Exclude>
auto
    FCk_Handle::
    Has_AnyLiveEntityWith_Excluding() const
    -> bool
{
    CK_ENSURE_IF_NOT(IsRegistryValid(),
        TEXT("Unable to perform Has_AnyLiveEntityWith_Excluding query with Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this)
    { return {}; }

    return Get_RegistryView().Has_AnyLiveEntityWith_Excluding<T_Fragment, T_Exclude...>();
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Get()
    -> T_Fragment&
{
    return Get<T_Fragment, ck::IsValid_Policy_Default>();
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Get() const
    -> const T_Fragment&
{
    // const& access is allowed on a PendingKill Entity because the returned Fragment is immutable
    return Get<T_Fragment, ck::IsValid_Policy_IncludePendingKill>();
}

template <typename T_Fragment, typename T_ValidationPolicy>
auto
    FCk_Handle::
    Get()
    -> T_Fragment&
{
    static_assert(std::is_base_of_v<ck::IsValid_Policy, T_ValidationPolicy>,
        "T_ValidationPolicy must be a ck::IsValid_Policy");

    static T_Fragment Invalid_Fragment;

    if (NOT IsValid(T_ValidationPolicy{}))
    {
        CK_ENSURE_IF_NOT(false,
            TEXT("Unable to Get Fragment [{}]. Handle [{}] {}."),
            ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
            [&]
            {
                if (IsRegistryValid())
                {
                    auto Reg = Get_RegistryView();
                    if (Reg.IsValid(_Entity))
                    { return TEXT("has an Entity that is about to be DESTROYED"); }

                    return TEXT("does NOT have a valid Entity");
                }
                return TEXT("does NOT have a valid Registry");
            }())
        { return Invalid_Fragment; }
    }

    CK_ENSURE_IF_NOT(Get_RegistryView().Has<T_Fragment>(_Entity),
        TEXT("Handle [{}] is missing Fragment [{}]. Returning Invalid_Fragment."),
        *this, ck::Get_RuntimeTypeToString<T_Fragment>())
    { return Invalid_Fragment; }

    return Get_RegistryView().Get<T_Fragment>(_Entity);
}

template <typename T_Fragment, typename T_ValidationPolicy>
auto
    FCk_Handle::
    Get() const
    -> const T_Fragment&
{
    static_assert(std::is_base_of_v<ck::IsValid_Policy, T_ValidationPolicy>,
        "T_ValidationPolicy must be a ck::IsValid_Policy");

    static T_Fragment Invalid_Fragment;

    if (NOT IsValid(T_ValidationPolicy{}))
    {
        CK_ENSURE_IF_NOT(false,
            TEXT("Unable to Get Fragment [{}]. Handle [{}] {}."),
            ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
            [&]
            {
                if (IsRegistryValid())
                {
                    auto Reg = Get_RegistryView();
                    if (Reg.IsValid(_Entity))
                    { return TEXT("has an Entity that is about to be DESTROYED"); }

                    return TEXT("does NOT have a valid Entity");
                }
                return TEXT("does NOT have a valid Registry");
            }())
        { return Invalid_Fragment; }
    }

    CK_ENSURE_IF_NOT(Get_RegistryView().Has<T_Fragment>(_Entity),
        TEXT("Handle [{}] is missing Fragment [{}]. Returning Invalid_Fragment."),
        *this, ck::Get_RuntimeTypeToString<T_Fragment>())
    { return Invalid_Fragment; }

    return Get_RegistryView().Get<T_Fragment>(_Entity);
}

template <typename T_Fragment>
requires (std::is_empty_v<T_Fragment>)
auto
    FCk_Handle::
    DoClear(
        bool InSkipQuarantined)
    -> void
{
    View<T_Fragment>().ForEach([&](EntityType InEntity)
    {
        auto Handle = ck::MakeHandle(InEntity, *this);
        if (InSkipQuarantined && Handle.Has<ck::FTag_Hydration_Quarantine>())
        { return; }
        Handle.DoRemove_FragmentDebugInfo<T_Fragment>();
    });
}

template <typename T_Fragment>
auto
    FCk_Handle::
    DoClear(
        bool InSkipQuarantined)
    -> void
{
    View<T_Fragment>().ForEach([&](EntityType InEntity, T_Fragment&)
    {
        auto Handle = ck::MakeHandle(InEntity, *this);
        if (InSkipQuarantined && Handle.Has<ck::FTag_Hydration_Quarantine>())
        { return; }
        Handle.DoRemove_FragmentDebugInfo<T_Fragment>();
    });
}

template <typename T_Fragment>
auto
    FCk_Handle::
    DoAdd_FragmentDebugInfo()
    -> void
{
    if (UCk_Utils_Ecs_Settings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Disable)
    { return; }

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    _Mapper = &Get_RegistryView().AddOrGet<FEntity_FragmentMapper>(_Entity);
    _Mapper->Add_FragmentInfo<T_Fragment>(*this);
#endif
}

template <typename T_Fragment>
auto
    FCk_Handle::
    DoRemove_FragmentDebugInfo()
    -> void
{
    if (UCk_Utils_Ecs_Settings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Disable)
    { return; }

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    _Mapper = &Get_RegistryView().AddOrGet<FEntity_FragmentMapper>(_Entity);
    _Mapper->Remove_FragmentInfo<T_Fragment>();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_ContextOwner
    {
        CK_GENERATED_BODY(FFragment_ContextOwner);

    public:
        using EntityType = FCk_Handle;

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

        CK_DEFINE_CONSTRUCTORS(FFragment_ContextOwner, _Entity);
    };
}

// --------------------------------------------------------------------------------------------------------------------

// FEntity_FragmentMapper::Add_FragmentInfo definition here instead of CkHandle_Debugging.h due to a circular dependency
template <typename T_Fragment>
auto
    FEntity_FragmentMapper::
    Add_FragmentInfo(const FCk_Handle& InHandle) const
    -> void
{
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    const auto FragmentInfo = [&]() -> DebugWrapperPtrType
    {
        if constexpr (std::is_empty_v<T_Fragment>)
        { return new TCk_DebugWrapper<T_Fragment>(nullptr); }
        else
        { return new TCk_DebugWrapper<T_Fragment>(&InHandle.Get<T_Fragment, ck::IsValid_Policy_IncludePendingKill>()); }
    }();

    if constexpr (std::is_empty_v<T_Fragment>)
    {
        _AllTags.Emplace(FragmentInfo);
        _TagNames.Emplace(FragmentInfo->Get_FragmentName(InHandle));
    }
    else if constexpr (std::is_same_v<DEBUG_NAME, T_Fragment>)
    {
        _DebugNameFragment = FragmentInfo;
        _DebugName = &InHandle.Get<DEBUG_NAME>()._Name;
    }
    else if constexpr (std::is_same_v<ck::FFragment_ContextOwner, T_Fragment>)
    {
        _Context = &InHandle.Get<T_Fragment, ck::IsValid_Policy_IncludePendingKill>();

        // ContextOwner stores only the fragment pointer, so the wrapper leaks unless freed here
        delete FragmentInfo;
    }
    else
    {
        _AllFragments.Emplace(FragmentInfo);
        _FragmentNames.Emplace(FragmentInfo->Get_FragmentName(InHandle));
    }

    // Lifetime tags stay in _AllTags too, so the debugger still shows two in flight at once (a bug)
    if constexpr (std::is_same_v<ck::FTag_DestroyEntity_Initiate, T_Fragment> ||
        std::is_same_v<ck::FTag_DestroyEntity_Teardown, T_Fragment> ||
        std::is_same_v<ck::FTag_DestroyEntity_Await, T_Fragment> ||
        std::is_same_v<ck::FTag_DestroyEntity_EndPlay, T_Fragment> ||
        std::is_same_v<ck::FTag_EntityJustCreated, T_Fragment>)
    {
       _LifetimeTag = FragmentInfo;
       _LifetimeTagName = ck::Get_LifetimeTagString<T_Fragment>();
    }
    else
    {
       _LifetimeTagName = ck::IsValid(InHandle) ? TEXT("Valid") : TEXT("Invalid");
    }

    if constexpr (std::is_same_v<ck::FTag_NetMode_IsHost, T_Fragment>)
    {
        _IsHost = true;
    }
    if constexpr (std::is_same_v<ck::FTag_NetMode_IsClient, T_Fragment>)
    {
        _IsClient = true;
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------
