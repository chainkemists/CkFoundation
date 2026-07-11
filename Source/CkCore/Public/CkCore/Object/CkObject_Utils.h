#pragma once

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Params.h"

#include "GameplayTagContainer.h"

#include "CkObject_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Utils_Object_RenameFlags : uint8
{
    None                  = 0,
    ForceNoResetLoaders   = 1,
    DoNotDirty            = 2,
    DontCreateRedirectors = 3,
    ForceGlobalUnique     = 4,
    SkipGeneratedClass    = 5
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Utils_Object_CopyAllProperties_Result : uint8
{
    Failed   ,
    Succeeded,
    AllPropertiesIdentical
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Utils_Object_CopyAllProperties_Result);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Utils_Object_CopyAllProperties_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_Object_CopyAllProperties_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Destination;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Source;

public:
    CK_PROPERTY(_Destination);
    CK_PROPERTY(_Source);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Object_SetOuter_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_Object_SetOuter_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Object;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Outer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Utils_Object_RenameFlags _RenameFlags = ECk_Utils_Object_RenameFlags::None;

public:
    CK_PROPERTY_GET(_Object);
    CK_PROPERTY(_Outer);
    CK_PROPERTY(_RenameFlags);

    CK_DEFINE_CONSTRUCTORS(FCk_Utils_Object_SetOuter_Params, _Object);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "UObject"))
class CKCORE_API UCk_Utils_Object_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Object_UE);

public:
    template <typename T_Object>
    static auto
    Get_ClassDefaultObject () -> T_Object*;

    template <typename T>
    static auto
    Get_ClassDefaultObject(
        TSubclassOf<UObject> InObject) -> T*;

    template <typename T_Object>
    static auto
    Get_ClassDefaultObject_UpToDate () -> T_Object*;

    template <typename T>
    static auto
    Get_ClassDefaultObject_UpToDate(
        TSubclassOf<UObject> InObject) -> T*;

    template <typename T>
    static auto
    Request_CloneObject(
        UObject* Outer,
        const T* InObjectToClone) -> T*;

    template <typename T>
    static auto
    Request_CloneObject(
        UObject* Outer,
        const T* InObjectToClone,
        TFunction<void(T*)> InPostClone) -> T*;

    template <typename T>
    static auto
    Request_CloneObject(
        const T* InObjectToClone,
        ck::policy::TransientPackage) -> T*;

    template <typename T>
    static auto
    Request_CloneObject(
        const T* InObjectToClone,
        TFunction<void(T*)> InPostClone,
        ck::policy::TransientPackage) -> T*;

    template<typename T>
    static auto
    Request_CreateNewObject(
        UObject* Outer,
        T* InTemplateArchetype,
        TFunction<void(T*)> InInitFunc) -> T*;

    template<typename T>
    static auto
    Request_CreateNewObject(
        UObject* Outer,
        TSubclassOf<T> InClass,
        T* InTemplateArchetype,
        TFunction<void(T*)> InInitFunc) -> T*;

    template<typename T>
    static auto
    Request_CreateNewObject(
        UObject* Outer,
        TSubclassOf<T> InClass,
        TFunction<void(T*)> InInitFunc) -> T*;

    // Pooling-aware create — the object-pooling hoist. The ObjectPooling subsystem of the Outer's
    // world OWNS the returned instance's lifetime (hold TWeakObjectPtr, never TStrongObjectPtr):
    // Recycle policy re-issues released instances with properties reset to InTemplateArchetype
    // (participant properties skipped); DestroyOnRelease pins until released, then lets GC collect.
    // Whether the instance is fresh or recycled is invisible to the caller. Release via
    // TryReleaseToPool. DestroyOnRelease instances are outered to Outer; Recycle-pool instances are
    // outered to Outer's world (a recycled instance survives its first caller, so it cannot borrow
    // that caller's outer). TransientPackage variants stay non-pooled (no world to resolve)
    template<typename T>
    static auto
    Request_CreateNewObject(
        UObject* Outer,
        TSubclassOf<T> InClass,
        T* InTemplateArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams,
        TFunction<void(T*)> InInitFunc) -> T*;

    template<typename T>
    static auto
    Request_CreateNewObject_TransientPackage(
        TFunction<void(T*)> InFunc) -> T*;

    template<typename T>
    static auto
    Request_CreateNewObject_TransientPackage() -> T*;

    template<typename T>
    static auto
    Request_CreateNewObject_TransientPackage(
        TSubclassOf<T> InClass) -> T*;

    template<typename T, typename T_Func>
    static auto
    Request_CreateNewObject_TransientPackage(
        TSubclassOf<T> InClass,
        T_Func InFunc) -> T*;

    // Strips UHT-generated prefixes/suffixes ("BP_" prefix, "_C" suffix) from a class name.
    // Returns "(unknown)" when InClass is null.
    static auto
    Get_CleanClassName(
        const UClass* InClass) -> FString;

    // Derives a gameplay tag from a class name by stripping the "_C" Blueprint CDO suffix and
    // replacing every '_' with '.'. Example: "Ck_SmState_Chase" -> "Ck.SmState.Chase".
    // InComment is forwarded to the gameplay tag resolver (used as the tag's description).
    // Returns an invalid FGameplayTag when InClass is null.
    static auto
    Get_TagFromClassName(
        const UClass* InClass,
        const FString& InComment) -> FGameplayTag;

    static auto
    Request_CallFunctionByName(
        UObject* InObject,
        FName InFunctionName,
        bool InEnsureFunctionExists = true) -> ECk_SucceededFailed;

    template<typename T>
    static auto
    Request_CallFunctionByNameWithParams(
        UObject* InObject,
        FName InFunctionName,
        T InParams,
        bool InEnsureFunctionExists = true) -> ECk_SucceededFailed;

    static auto
    ForEach_ObjectsWithOuter(
        const UObject* InOuterObject,
        TFunction<void(UObject*)> InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Get Generated Unique Object Name",
              Category = "Ck|Utils|Object")
    static FName
    Get_GeneratedUniqueName(
        UObject* InThis,
        UClass* InObj,
        FName InBaseName);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Request Try Set Object Outer",
              Category = "Ck|Utils|Object")
    static ECk_SucceededFailed
    Request_TrySetOuter(
        const FCk_Utils_Object_SetOuter_Params& InParams);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Request Copy All Object Properties",
              Category = "Ck|Utils|Object")
    static ECk_Utils_Object_CopyAllProperties_Result
    Request_CopyAllProperties(
        const FCk_Utils_Object_CopyAllProperties_Params& InParams);

    // Reset all the properties of a UObject to the value assigned in the CDO.
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Request Reset All Object Properties To Default",
              Category = "Ck|Utils|Object")
    static ECk_SucceededFailed
    Request_ResetAllPropertiesToDefault(
        UObject* InObject);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Request Create New Object",
              Category = "Ck|Utils|Object",
              meta     = (DeterminesOutputType = "InObject"))
    static UObject*
    Request_CreateNewObject_TransientPackage_UE(
        TSubclassOf<UObject> InObject);

    // Pooling-aware create (see the template overload above for the ownership contract).
    // InTemplateArchetype may be null (class CDO is the archetype)
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Request Create New Object (Pooled)",
              Category = "Ck|Utils|Object",
              meta     = (DeterminesOutputType = "InClass"))
    static UObject*
    Request_CreateNewObject_Pooled(
        UObject* InOuter,
        TSubclassOf<UObject> InClass,
        UObject* InTemplateArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams);

    // Release a pooling-subsystem-managed object: Recycle-policy instances park in their pool
    // (participant OnReleasedToPool fires); DestroyOnRelease instances are unpinned and left to GC.
    // A benign no-op (returns Failed) for objects the subsystem never handed out — safe to call
    // unconditionally from any teardown path
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Try Release Object To Pool",
              Category = "Ck|Utils|Object")
    static ECk_SucceededFailed
    TryReleaseToPool(
        UObject* InObject);

    // True when the ObjectPooling subsystem of the object's world handed out (and still tracks) this
    // instance — pooled in-use OR pinned-unique. CDOs and objects created outside the pooling-aware
    // path return false. Use to gate TryReleaseToPool without tripping its never-handed-out ensure
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Pool-Tracked Object",
              Category = "Ck|Utils|Object")
    static bool
    Get_IsPoolTrackedObject(
        const UObject* InObject);

    // Stats snapshot for the (class, archetype) pool — zeroed struct when no such pool exists.
    // Null archetype resolves to the class CDO, mirroring the acquire path's pool keying.
    // InWorldContextObject resolves the world (no WorldContext meta — it conflicts with the
    // ScriptMixin=UObject arg0-receiver binding; pass the object explicitly in BP/AS)
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Object Pool Stats",
              Category = "Ck|Utils|Object")
    static FCk_ObjectPooling_PoolStats
    Get_ObjectPoolStats(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InClass,
        UObject* InArchetype);

    // Unreal prefixes some classes with REINST_. This is because REINST_ is a newer version of a static class.
    // This function gets the most up to date default class.
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Default Class (Up-To-Date)",
              Category = "Ck|Utils|Object")
    static UClass*
    Get_DefaultClass_UpToDate(
        UClass* InClass);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Class Default Object",
              Category = "Ck|Utils|Object")
    static bool
    Get_IsDefaultObject(
        const UObject* InObject);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Archetype Object",
              Category = "Ck|Utils|Object")
    static bool
    Get_IsArchetypeObject(
        const UObject* InObject);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Object Native Parent Class",
              Category = "Ck|Utils|Object")
    static UClass*
    Get_ObjectNativeParentClass(
        const UObject* InObject);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Blueprint Generated Class",
              Category = "Ck|Utils|Object")
    static UClass*
    Get_BlueprintGeneratedClass(
        const UObject* InBlueprintObject);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Class Generated By Blueprint",
              Category = "Ck|Utils|Object")
    static UObject*
    Get_ClassGeneratedByBlueprint(
        const UClass* InBlueprintGeneratedClass);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Derived Classes Of",
              Category = "Ck|Utils|Object")
    static TArray<UClass*>
    Get_DerivedClasses(
        const UClass* InBaseClass,
        bool InRecursive);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Cast Object to Interface",
        meta = (CompactNodeTitle = "<AsInterface>", BlueprintAutocast))
    static TScriptInterface<UInterface>
    Cast_ObjectToInterface(
        UObject* InInterfaceObject);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Cast Class to Interface",
        meta = (CompactNodeTitle = "<AsInterface>", BlueprintAutocast))
    static TSubclassOf<UInterface>
    Cast_ClassToInterface(
        TSubclassOf<UClass> InInterfaceClass);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object",
              DisplayName = "[Ck] Get Class Default Object",
              meta     = (DeterminesOutputType = "InObject"))
    static UObject*
    DoGet_ClassDefaultObject(
        TSubclassOf<UObject> InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object",
              DisplayName = "[Ck] Get Class Default Object (Up-To-Date)",
              meta     = (DeterminesOutputType = "InObject"))
    static UObject*
    DoGet_ClassDefaultObject_UpToDate(
        TSubclassOf<UObject> InObject);

    static auto
    DoRequest_CallFunctionByName(
        UObject* InObject,
        FName InFunctionName,
        bool InEnsureFunctionExists = true,
        void* InFunctionParams = nullptr) -> ECk_SucceededFailed;

    // Non-template plumbing for the pooled create: resolves the Outer's world and forwards to its
    // ObjectPooling subsystem. No resolvable world fires an ensure and falls back to a plain
    // (non-pooled, caller-owned) create so gameplay continues
    static auto
    DoRequest_AcquirePooled(
        UObject* InOuter,
        TSubclassOf<UObject> InClass,
        UObject* InTemplateArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams) -> UObject*;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Object>
auto
    UCk_Utils_Object_UE::
    Get_ClassDefaultObject()
    -> T_Object*
{
    return Get_ClassDefaultObject<T_Object>(T_Object::StaticClass());
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Get_ClassDefaultObject(
        TSubclassOf<UObject> InObject)
    -> T*
{
    return Cast<T>(DoGet_ClassDefaultObject(InObject));
}

template <typename T_Object>
auto
    UCk_Utils_Object_UE::
    Get_ClassDefaultObject_UpToDate()
    -> T_Object*
{
    return Get_ClassDefaultObject_UpToDate<T_Object>(T_Object::StaticClass());
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Get_ClassDefaultObject_UpToDate(
        TSubclassOf<UObject> InObject)
    -> T*
{
    return Cast<T>(DoGet_ClassDefaultObject_UpToDate(InObject));
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        UObject* Outer,
        const T* InObjectToClone)
    -> T*
{
    static_assert(NOT std::is_base_of_v<AActor, T>, "Request_CloneObject cannot be used on an AActor. Use Request_CloneActor instead");

    CK_ENSURE_IF_NOT(ck::IsValid(InObjectToClone), TEXT("Unable to CloneObject with Outer [{}]. Object to clone is INVALID."), Outer)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InObjectToClone->template IsA<const AActor>(),
        TEXT("Unable to clone [{}] with Outer [{}]. Request_CloneObject cannot be used on an AActor. Use Request_CloneActor instead."),
        InObjectToClone, Outer)
    { return {}; }

    const auto& SafeName = Get_GeneratedUniqueName(Outer, InObjectToClone->GetClass(), NAME_None);

    auto ClonedObject =  DuplicateObject<T>(InObjectToClone, Outer, SafeName);

    if constexpr (TIsDerivedFrom<T, USceneComponent>::IsDerived)
    {
        if (auto ClonedObjectAsSceneComp = Cast<USceneComponent>(ClonedObject);
            ck::IsValid(ClonedObjectAsSceneComp))
        {
            ClonedObjectAsSceneComp->RegisterComponent();
            ClonedObjectAsSceneComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
        }
    }

    return ClonedObject;
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        UObject* Outer,
        const T* InObjectToClone,
        TFunction<void(T*)> InPostClone)
    -> T*
{
    auto NewObj = Request_CloneObject(Outer, InObjectToClone);

    if (InPostClone)
    {
        InPostClone(NewObj);
    }

    return NewObj;
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        const T* InObjectToClone,
        ck::policy::TransientPackage)
    -> T*
{
    return Request_CloneObject(GetTransientPackage(), InObjectToClone);
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        const T* InObjectToClone,
        TFunction<void(T*)> InPostClone,
        ck::policy::TransientPackage)
    -> T*
{
    return Request_CloneObject(GetTransientPackage(), InObjectToClone, InPostClone);
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject(
        UObject* Outer,
        T* InTemplateArchetype,
        TFunction<void(T*)> InInitFunc)
    -> T*
{
    auto* NewObj = NewObject<T>
    (
        Outer,
        T::StaticClass(),
        NAME_None,
        RF_NoFlags,
        InTemplateArchetype
    );

    if (InInitFunc)
    {
        InInitFunc(NewObj);
    }

    return NewObj;
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject(
        UObject* Outer,
        TSubclassOf<T> InClass,
        T* InTemplateArchetype,
        TFunction<void(T*)> InInitFunc)
    -> T*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass), TEXT("Invalid Class supplied to Request_CreateNewObject"))
    { return {}; }

    auto* NewObj = NewObject<T>
    (
        Outer,
        InClass,
        NAME_None,
        RF_NoFlags,
        InTemplateArchetype
    );

    if (InInitFunc)
    {
        InInitFunc(NewObj);
    }

    return NewObj;
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject(
        UObject* Outer,
        TSubclassOf<T> InClass,
        TFunction<void(T*)> InInitFunc)
    -> T*
{
    return Request_CreateNewObject<T>(Outer, InClass, nullptr, InInitFunc);
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject(
        UObject* Outer,
        TSubclassOf<T> InClass,
        T* InTemplateArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams,
        TFunction<void(T*)> InInitFunc)
    -> T*
{
    auto* Acquired = DoRequest_AcquirePooled(Outer, InClass, InTemplateArchetype, InPoolParams);

    auto* TypedObj = Cast<T>(Acquired);

    if (InInitFunc && ck::IsValid(TypedObj))
    {
        InInitFunc(TypedObj);
    }

    return TypedObj;
}

template<typename T>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject_TransientPackage(
        TFunction<void(T*)> InFunc)
    -> T*
{
    return Request_CreateNewObject<T>(GetTransientPackage(), nullptr, InFunc);
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject_TransientPackage()
    -> T*
{
    return Request_CreateNewObject<T>(GetTransientPackage(), nullptr, [](T*){});
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject_TransientPackage(
        TSubclassOf<T> InClass)
    -> T*
{
    return Request_CreateNewObject_TransientPackage(InClass, nullptr);
}

template <typename T, typename T_Func>
auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject_TransientPackage(
        TSubclassOf<T> InClass,
        T_Func InFunc)
    -> T*
{
    return Request_CreateNewObject<T>(GetTransientPackage(), InClass, nullptr, InFunc);
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CallFunctionByNameWithParams(
        UObject* InObject,
        FName InFunctionName,
        T InParams,
        bool InEnsureFunctionExists)
    -> ECk_SucceededFailed
{
    return DoRequest_CallFunctionByName(InObject, InFunctionName, InEnsureFunctionExists, &InParams);
}

// --------------------------------------------------------------------------------------------------------------------