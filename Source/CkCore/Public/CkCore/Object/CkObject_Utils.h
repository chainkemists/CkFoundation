#pragma once

#include "CkCore/Ensure/CkEnsure.h"

#include "CkCore/Macros/CkMacros.h"

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
    CK_PROPERTY_GET(_Outer);
    CK_PROPERTY_GET(_RenameFlags);

    CK_DEFINE_CONSTRUCTORS(FCk_Utils_Object_SetOuter_Params, _Object, _Outer, _RenameFlags);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Object")
    static bool
    Request_TrySetOuter(
        const FCk_Utils_Object_SetOuter_Params& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Object")
    static ECk_Utils_Object_CopyAllProperties_Result
    Request_CopyAllProperties(
        const FCk_Utils_Object_CopyAllProperties_Params& InParams);

    // Reset all the properties of a UObject to the value assigned in the CDO.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Object")
    static bool
    Request_ResetAllPropertiesToDefault(
        UObject* InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object",
              meta     = (DeterminesOutputType = "InObject", DisplayName = "Request_CreateNewObject"))
    static UObject*
    Request_CreateNewObject_TransientPackage_UE(
        TSubclassOf<UObject> InObject);

    // Unreal prefixes some classes with REINST_. This is because REINST_ is a newer version of a static class.
    // This function gets the most up to date default class.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object")
    static UClass*
    Get_DefaultClass_UpToDate(
        UClass* InClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object")
    static bool
    Get_IsDefaultObject(
        UObject* InObject);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object",
              DisplayName = "Get_ClassDefaultObject",
              meta     = (DeterminesOutputType = "InObject"))
    static UObject*
    DoGet_ClassDefaultObject(
        TSubclassOf<UObject> InObject);
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
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Class supplied to Get_ClassDefaultObject"))
    { return {}; }

    return Cast<T>(InObject->GetDefaultObject());
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

    auto NonConstObjectToClone = const_cast<T*>(InObjectToClone);
    const auto& Class = NonConstObjectToClone->GetClass();

    return NewObject<T>(Outer, Class, NAME_None, RF_NoFlags, NonConstObjectToClone);
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
    auto* newObj = NewObject<T>
    (
        Outer,
        T::StaticClass(),
        NAME_None,
        RF_NoFlags,
        InTemplateArchetype
    );

    if (InInitFunc)
    {
        InInitFunc(newObj);
    }

    return newObj;
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

    auto* newObj = NewObject<T>
    (
        Outer,
        InClass,
        NAME_None,
        RF_NoFlags,
        InTemplateArchetype
    );

    if (InInitFunc)
    {
        InInitFunc(newObj);
    }

    return newObj;
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

// --------------------------------------------------------------------------------------------------------------------
