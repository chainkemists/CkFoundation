#include "CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Data>
struct FDummyTemplatedData
{
    CK_ENABLE_CUSTOM_VALIDATION();

public:
    using value_type = T_Data;
    FDummyTemplatedData(value_type InTemplatedData)
    : _TemplatedData(InTemplatedData)
    {}

private:
    value_type _TemplatedData;
};

CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(FDummyTemplatedData);

CK_DEFINE_CUSTOM_IS_VALID_T(T, FDummyTemplatedData<T>, ck::IsValid_Policy_Default,
[=](const FDummyTemplatedData<T>& InDummyTemplatedData)
{
    return ck::IsValid(InDummyTemplatedData._TemplatedData);
});

// --------------------------------------------------------------------------------------------------------------------

struct FDummyData
{
    CK_ENABLE_CUSTOM_VALIDATION();

public:
    FDummyData(AActor* InActor)
        : _Actor(InActor)
    {}

private:
    AActor* _Actor = nullptr;
};

CK_DEFINE_CUSTOM_IS_VALID(FDummyData, ck::IsValid_Policy_Default,
[=](const FDummyData& InDummyData)
{
    return ck::IsValid(InDummyData._Actor);
});

// --------------------------------------------------------------------------------------------------------------------

struct FDummyStruct
{
    CK_ENABLE_CUSTOM_VALIDATION();

public:
    FDummyStruct(FDummyData InData)
        : _DataToValidate(InData)
    {}

private:
    FDummyData _DataToValidate;
};

CK_DEFINE_CUSTOM_IS_VALID(FDummyStruct, ck::IsValid_Policy_Default,
[=](const FDummyStruct& InDummyStruct)
{
    return ck::IsValid(InDummyStruct._DataToValidate);
});

// --------------------------------------------------------------------------------------------------------------------

namespace ck::Private
{
    auto IsValid_CompileTest() -> void
    {
        // AActor Validation
        AActor* Actor = nullptr;
        ck::IsValid(Actor);
        ck::IsValid(Actor, ck::IsValid_Policy_IncludePendingKill{});
        ck::IsValid(Actor, ck::IsValid_Policy_NullptrOnly{});
        ck::Is_NOT_Valid(Actor);
        ck::Is_NOT_Valid(Actor, ck::IsValid_Policy_IncludePendingKill{});
        ck::Is_NOT_Valid(Actor, ck::IsValid_Policy_NullptrOnly{});

        // Double pointer
        AActor** ActorDblPtr = nullptr;
        ck::IsValid(ActorDblPtr, ck::IsValid_Policy_NullptrOnly{});
        ck::Is_NOT_Valid(ActorDblPtr, ck::IsValid_Policy_NullptrOnly{});

        // FField Validation
        FField* Field = nullptr;
        ck::IsValid(Field);
        ck::Is_NOT_Valid(Field);

        // UObject Validation
        UObject* Object = nullptr;
        ck::IsValid(Object);
        ck::Is_NOT_Valid(Object);

        // TSubclassOf Validation
        const auto SubclassOfActor = TSubclassOf<AActor>{};
        ck::IsValid(SubclassOfActor);
        ck::Is_NOT_Valid(SubclassOfActor);

        // TSharedPtr Validation
        {
            const TSharedPtr<AActor> SharedPtrOfActor = nullptr;
            ck::IsValid(SharedPtrOfActor);
            ck::Is_NOT_Valid(SharedPtrOfActor);
        }
        {
            const TSharedPtr<AActor, ESPMode::NotThreadSafe> SharedPtrOfActor = nullptr;
            ck::IsValid(SharedPtrOfActor);
            ck::Is_NOT_Valid(SharedPtrOfActor);
        }

        // TWeakPtr Validation
        {
            const TWeakPtr<AActor> WeakPtrOfActor = nullptr;
            ck::IsValid(WeakPtrOfActor);
            ck::Is_NOT_Valid(WeakPtrOfActor);
        }
        {
            const TWeakPtr<AActor, ESPMode::NotThreadSafe> WeakPtrOfActor = nullptr;
            ck::IsValid(WeakPtrOfActor);
            ck::Is_NOT_Valid(WeakPtrOfActor);
        }

        // TOptional Validation
        const TOptional<AActor*> OptionalOfActor = nullptr;
        ck::IsValid(OptionalOfActor);
        ck::Is_NOT_Valid(OptionalOfActor);

        // FName Validation
        const FName Name = NAME_None;
        ck::IsValid(Name);
        ck::Is_NOT_Valid(Name);

        // FName Validation
        const FSoftObjectPath SoftObjPath;
        ck::IsValid(SoftObjPath);
        ck::Is_NOT_Valid(SoftObjPath);

        // FGameplayTag Validation
        const FGameplayTag GameplayTag = FGameplayTag::EmptyTag;
        ck::IsValid(GameplayTag);
        ck::Is_NOT_Valid(GameplayTag);

        // FGuid Validation
        const FGuid GUID;
        ck::IsValid(GUID);
        ck::Is_NOT_Valid(GUID);

        // Custom Data Type Validation
        const auto DummyData = FDummyData{Actor};
        const auto DummyStruct = FDummyStruct{DummyData};
        ck::IsValid(DummyData);
        ck::Is_NOT_Valid(DummyData);
        ck::IsValid(DummyStruct);
        ck::Is_NOT_Valid(DummyStruct);

        // Custom Templated Data Type Validation
        const auto DummyTemplatedData = FDummyTemplatedData{Actor};
        ck::IsValid(DummyTemplatedData);
        ck::Is_NOT_Valid(DummyTemplatedData);
    }
}

// --------------------------------------------------------------------------------------------------------------------
