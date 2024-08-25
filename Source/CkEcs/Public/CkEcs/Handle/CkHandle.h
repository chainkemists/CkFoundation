#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle_Debugging.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Settings/CkEcs_Settings.h"

#include "CkHandle.generated.h"

// --------------------------------------------------------------------------------------------------------------------

#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING

// --------------------------------------------------------------------------------------------------------------------

// enable pointer stability for all Fragments when debugging
template<typename Type>
struct entt::component_traits<Type> {
    // ReSharper disable once CppInconsistentNaming
    static constexpr bool in_place_delete = true;
    // ReSharper disable once CppInconsistentNaming
    static constexpr std::size_t page_size = !std::is_empty_v<Type> * ENTT_PACKED_PAGE;
};

// --------------------------------------------------------------------------------------------------------------------

struct DEBUG_NAME
{
    friend class UCk_Utils_Handle_UE;
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

#endif

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(NoImplicitConversion, HasNativeMake, HasNativeBreak="/Script/CkEcs.Ck_Utils_Handle_UE:Break_Handle"))
struct CKECS_API FCk_Handle
{
    GENERATED_BODY()

    friend class UCk_Utils_EntityReplicationDriver_UE;
    friend class UCk_Fragment_EntityReplicationDriver_Rep;

public:
    CK_GENERATED_BODY(FCk_Handle);
    CK_ENABLE_CUSTOM_FORMATTER(FCk_Handle);

public:
    using EntityType = FCk_Entity;
    using RegistryType = FCk_Registry;

public:
    FCk_Handle() = default;
    FCk_Handle(EntityType InEntity, const RegistryType& InRegistry);

    FCk_Handle(ThisType&& InOther) noexcept;
    FCk_Handle(const ThisType& InOther);

    // this is a special hard-coded function that expects the type-safe handle to have a particular function
    template <typename T_WrappedHandle, class = std::enable_if_t<std::is_base_of_v<struct FCk_Handle_TypeSafe, T_WrappedHandle>>>
    FCk_Handle(const T_WrappedHandle& InTypeSafeHandle);

    auto operator=(ThisType InOther) -> ThisType&;
    // auto operator=(ThisType&& InOther) -> ThisType&; // intentionally not implemented

    ~FCk_Handle();

public:
    auto Swap(ThisType& InOther) -> void;

public:
    // this is a special hard-coded function that expects the type-safe handle to have a particular function
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

    template <typename T_Fragment>
    auto Remove() -> void;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Remove() -> void;

    template <typename T_Fragment, typename T_FragmentFunc>
    auto CopyAndRemove(T_Fragment FragmentToCopyAndRemove, T_FragmentFunc InFunc) -> void;

    template <typename T_Fragment>
    auto Try_Remove() -> void;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Try_Remove() -> void;

    template <typename... T_Fragments>
    auto Clear() -> void;

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

    template <typename T_Fragment>
    auto Get() -> T_Fragment&;

    template <typename T_Fragment>
    auto Get() const -> const T_Fragment&;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() -> T_Fragment&;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() const -> const T_Fragment&;

public:
    // FCk_Handle is a core concept of this architecture, we are taking some liberties
    // with the operator overloading for the sake of improving readability. Overloading
    // operators like this for non-core types is generally forbidden.
    auto operator*() -> TOptional<FCk_Registry>;
    auto operator*() const -> TOptional<FCk_Registry>;

    auto operator->() -> TOptional<FCk_Registry>;
    auto operator->() const -> TOptional<FCk_Registry>;

public:
    auto IsValid(ck::IsValid_Policy_Default) const -> bool;
    auto IsValid(ck::IsValid_Policy_IncludePendingKill) const -> bool;

    auto Orphan() const -> bool;
    auto Get_ValidHandle(EntityType::IdType InEntity) const -> ThisType;

    auto Get_Registry() -> FCk_Registry&;
    auto Get_Registry() const -> const FCk_Registry&;

public:
    auto
    NetSerialize(
        FArchive& Ar,
        class UPackageMap* Map,
        bool& bOutSuccess) -> bool;

private:
    template <typename T_Fragment>
    requires(std::is_empty_v<T_Fragment>)
    auto DoClear() -> void;

    template <typename T_Fragment>
    auto DoClear() -> void;

    auto DoUpdate_FragmentDebugInfo_Blueprints() -> void;

    template <typename T_Fragment>
    auto DoAdd_FragmentDebugInfo() -> void;

    template <typename T_Fragment>
    auto DoRemove_FragmentDebugInfo() -> void;

protected:
    UPROPERTY()
    FCk_Entity _Entity;

    TOptional<FCk_Registry> _Registry;

private:
    UPROPERTY()
    TWeakObjectPtr<class UCk_Ecs_ReplicatedObject_UE> _ReplicationDriver;

#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    const struct FEntity_FragmentMapper* _Mapper = nullptr;
#endif

public:
    CK_PROPERTY(_Entity);

#if WITH_EDITORONLY_DATA
private:
    UPROPERTY(Transient) // needs to be a UPROPERTY so that it shows up when debugging Blueprints
    TWeakObjectPtr<class UCk_Handle_FragmentsDebug> _Fragments = nullptr;
#endif
};

// --------------------------------------------------------------------------------------------------------------------

template<>
struct TStructOpsTypeTraits<FCk_Handle> : public TStructOpsTypeTraitsBase2<FCk_Handle>
{
    enum
    { WithNetSerializer = true };
};

// --------------------------------------------------------------------------------------------------------------------

#define CK_IF_HANDLE_IS_PENDING_KILL(_Handle_)\
if (ck::Is_NOT_Valid(_Handle_) && ck::IsValid(_Handle_, ck::IsValid_Policy_IncludePendingKill{}))

// --------------------------------------------------------------------------------------------------------------------

auto CKECS_API GetTypeHash(const FCk_Handle& InHandle) -> uint32;

namespace ck
{
    // Having an overload with FCK_Handle helps usages where a type T that could be an Entity or Handle is used and the code
    // does not know or care. Another case is when we use FCk_Entity in debug builds and FCk_Handle in release
    // builds but the code that uses the handle (or entity) does not care and wants a handle back

    auto CKECS_API MakeHandle(FCk_Entity InEntity, FCk_Handle InValidHandle) -> FCk_Handle;
    auto CKECS_API MakeHandle(FCk_Handle InEntity, FCk_Handle InValidHandle) -> FCk_Handle;

    auto CKECS_API IsValid(FCk_Entity InEntity, FCk_Handle InValidHandle) -> bool;
    auto CKECS_API IsValid(FCk_Handle InEntity, FCk_Handle InValidHandle) -> bool;

    auto CKECS_API GetEntity(FCk_Entity InEntity) -> FCk_Entity;
    auto CKECS_API GetEntity(FCk_Handle InEntity) -> FCk_Entity;
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

CK_DEFINE_CUSTOM_IS_VALID(FCk_Handle, ck::IsValid_Policy_Default, [&](const FCk_Handle& InHandle)
{
    return InHandle.IsValid(ck::IsValid_Policy_Default{});
});

CK_DEFINE_CUSTOM_IS_VALID(FCk_Handle, ck::IsValid_Policy_IncludePendingKill, [&](const FCk_Handle& InHandle)
{
    return InHandle.IsValid(ck::IsValid_Policy_IncludePendingKill{});
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS(FCk_Handle,
[&]()
{
    if (ck::IsValid(InObj, ck::IsValid_Policy_IncludePendingKill{}) && InObj.Has<DEBUG_NAME>())
    { return ck::Format(TEXT("{}({})"), InObj.Get_Entity(), InObj.Get<DEBUG_NAME>().Get_Name()); }

    return ck::Format(TEXT("{}"), InObj.Get_Entity());
},
[&]()
{
    if (ck::IsValid(InObj, ck::IsValid_Policy_IncludePendingKill{}) && InObj.Has<DEBUG_NAME>())
    { return ck::Format(TEXT("{}[{}]({})"), InObj._Entity, InObj.Get_Registry(), InObj.Get<DEBUG_NAME>().Get_Name()); }

    return ck::Format(TEXT("{}({})"), InObj._Entity, InObj.Get_Registry());
});

// --------------------------------------------------------------------------------------------------------------------

template <typename T_WrappedHandle, class>
FCk_Handle::
    FCk_Handle(
        const T_WrappedHandle& InOther)
    : _Entity(InOther._Entity)
    , _Registry(InOther._Registry)
    , _ReplicationDriver(InOther._ReplicationDriver)
#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
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
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    auto& NewFragment = _Registry->Add<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

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
            if (ck::Is_NOT_Valid(_Registry))
            { return TEXT("does NOT have a valid Registry"); }

            if (NOT _Registry->IsValid(_Entity))
            { return TEXT("does NOT have a valid Entity"); }

            return TEXT("");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    auto& NewOrExistingFragment = [&]() -> T_Fragment&
    {
        // only add it ONCE in the debugger
        const auto AddDebugInfo = NOT Has<T_Fragment>();

        auto& Fragment = _Registry->AddOrGet<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

        if (AddDebugInfo)
        {
            auto& FragmentToReturn = _Registry->AddOrGet<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

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
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
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
        // only add it ONCE in the debugger
        const auto AddDebugInfo = NOT Has<T_Fragment>();

        auto& Fragment = _Registry->AddOrGet<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

        if (AddDebugInfo)
        {
            auto& FragmentToReturn = _Registry->AddOrGet<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);

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
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    { return; }

    _Registry->Try_Transform<T_Fragment>(_Entity, InFunc);
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
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return _Registry->Replace<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
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
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return _Registry->AddOrReplace<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Remove()
    -> void
{
    Remove<T_Fragment, ck::IsValid_Policy_Default>();
}

template <typename T_Fragment, typename T_ValidationPolicy>
auto
    FCk_Handle::
    Remove()
    -> void
{
    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    { return; }

    _Registry->Remove<T_Fragment>(_Entity);

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
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
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
    -> void
{
    Try_Remove<T_Fragment, ck::IsValid_Policy_Default>();
}

template <typename T_Fragment, typename T_ValidationPolicy>
auto
    FCk_Handle::
    Try_Remove()
    -> void
{
    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    { return; }

    _Registry->Try_Remove<T_Fragment>(_Entity);

    DoRemove_FragmentDebugInfo<T_Fragment>();
}

template <typename ... T_Fragments>
auto
    FCk_Handle::
    Clear()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Clear<...> Fragments. Handle [{}] does NOT have a valid Registry."), *this)
    { return; }

    // we need to do this to allow Entity debugging to properly clear the debug mapping
#if CK_ECS_DISABLE_HANDLE_DEBUGGING == 0
    (DoClear<T_Fragments>(), ...);
#endif
    _Registry->Clear<T_Fragments...>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    View()
    -> FCk_Registry::RegistryViewType<T_Fragment...>
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static RegistryType Invalid_Registry;
        return Invalid_Registry.View<T_Fragment...>();
    }

    return _Registry->View<T_Fragment...>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    View() const
    -> FCk_Registry::ConstRegistryViewType<T_Fragment...>
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static RegistryType Invalid_Registry;
        return Invalid_Registry.View<T_Fragment...>();
    }

    return _Registry->View<T_Fragment...>();
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Has() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has query with Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this)
    { return {}; }

    return _Registry->Has<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    Has_Any() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_Any query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has_Any<T_Fragment...>(_Entity);
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    Has_All() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_All query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has_All<T_Fragment...>(_Entity);
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
    // const & getter is allowed to get Fragments even if Entity is PendingKill because the Fragment
    // returned is immutable
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

    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to Get Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return _Registry->Get<T_Fragment>(_Entity);
}

template <typename T_Fragment, typename T_ValidationPolicy>
auto
    FCk_Handle::
    Get() const
    -> const T_Fragment&
{
    static_assert(std::is_base_of_v<ck::IsValid_Policy, T_ValidationPolicy>,
        "T_ValidationPolicy must be a ck::IsValid_Policy");

    CK_ENSURE_IF_NOT(IsValid(T_ValidationPolicy{}),
        TEXT("Unable to Get Fragment [{}]. Handle [{}] {}."),
        ck::Get_RuntimeTypeToString<T_Fragment>(), *this,
        [&]
        {
            if (ck::IsValid(_Registry))
            {
                if (_Registry->IsValid(_Entity))
                { return TEXT("has an Entity that is about to be DESTROYED"); }

                return TEXT("does NOT have a valid Entity");
            }
            return TEXT("does NOT have a valid Registry");
        }())
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return _Registry->Get<T_Fragment>(_Entity);
}

template <typename T_Fragment>
requires (std::is_empty_v<T_Fragment>)
auto
    FCk_Handle::
    DoClear()
    -> void
{
    View<T_Fragment>().ForEach([&](EntityType InEntity)
    {
        auto Handle = ck::MakeHandle(InEntity, *this);
        Handle.DoRemove_FragmentDebugInfo<T_Fragment>();
    });
}

template <typename T_Fragment>
auto
    FCk_Handle::
    DoClear()
    -> void
{
    View<T_Fragment>().ForEach([&](EntityType InEntity, T_Fragment&)
    {
        auto Handle = ck::MakeHandle(InEntity, *this);
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

#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    _Mapper = &_Registry->AddOrGet<FEntity_FragmentMapper>(_Entity);
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

#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    _Mapper = &_Registry->AddOrGet<FEntity_FragmentMapper>(_Entity);
    _Mapper->Remove_FragmentInfo<T_Fragment>();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

// FEntity_FragmentMapper::Add_FragmentInfo definition here instead of CkHandle_Debugging.h due to a circular dependency
template <typename T_Fragment>
auto
    FEntity_FragmentMapper::
    Add_FragmentInfo(const FCk_Handle& InHandle) const
    -> void
{
    auto FragmentInfo = DebugWrapperPtrType{};
    if constexpr (std::is_empty_v<T_Fragment>)
    {
        FragmentInfo = std::make_shared<TCk_DebugWrapper<T_Fragment>>(nullptr);
    }
    else
    {
        FragmentInfo = std::shared_ptr<TCk_DebugWrapper<T_Fragment>>
        { new TCk_DebugWrapper<T_Fragment>(&InHandle.Get<T_Fragment, ck::IsValid_Policy_IncludePendingKill>()) };
    }

    _FragmentNames.Emplace(FragmentInfo->Get_FragmentName(InHandle));
    _AllFragments.Emplace(MoveTemp(FragmentInfo));
}

// --------------------------------------------------------------------------------------------------------------------
