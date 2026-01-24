#pragma once

#include "CkEcs/Handle/CkHandle.h"

#if WITH_ANGELSCRIPT_CK

#include <AngelscriptType.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Custom AngelScript type implementation for dynamic handles.
 * Allows dynamic handle types to be used as UPROPERTYs by storing them as FCk_Handle.
 */
class CKDYNAMIC_API FCkDynamic_AngelscriptType : public FAngelscriptType
{
public:
    FCkDynamic_AngelscriptType(
        const FString& InTypeName,
        const FString& InShortName);

    virtual auto GetAngelscriptTypeName() const -> FString override;
    virtual auto GetAngelscriptTypeName(const FAngelscriptTypeUsage& Usage) const -> FString override;

    virtual auto GetClass(const FAngelscriptTypeUsage& Usage) const -> UClass* override;
    virtual auto GetUnrealStruct(const FAngelscriptTypeUsage& Usage) const -> UStruct* override;

    virtual auto MatchesProperty(
        const FAngelscriptTypeUsage& Usage,
        const FProperty* Property,
        EPropertyMatchType MatchType) const -> bool override;

    virtual auto CanCreateProperty(const FAngelscriptTypeUsage& Usage) const -> bool override;

    virtual auto CreateProperty(
        const FAngelscriptTypeUsage& Usage,
        const FPropertyParams& Params) const -> FProperty* override;

    virtual auto HasReferences(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto NeverRequiresGC(const FAngelscriptTypeUsage& Usage) const -> bool override;

    virtual auto CanCopy(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto NeedCopy(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto CopyValue(
        const FAngelscriptTypeUsage& Usage,
        void* SourcePtr,
        void* DestinationPtr) const -> void override;

    virtual auto CanCompare(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto IsValueEqual(
        const FAngelscriptTypeUsage& Usage,
        void* SourcePtr,
        void* DestinationPtr) const -> bool override;

    virtual auto CanConstruct(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto NeedConstruct(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto ConstructValue(
        const FAngelscriptTypeUsage& Usage,
        void* DestinationPtr) const -> void override;

    virtual auto CanDestruct(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto NeedDestruct(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto DestructValue(
        const FAngelscriptTypeUsage& Usage,
        void* DestinationPtr) const -> void override;

    virtual auto GetValueSize(const FAngelscriptTypeUsage& Usage) const -> int32 override;
    virtual auto GetValueAlignment(const FAngelscriptTypeUsage& Usage) const -> int32 override;

    virtual auto CanBeArgument(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto SetArgument(
        const FAngelscriptTypeUsage& Usage,
        int32 ArgumentIndex,
        asIScriptContext* Context,
        FFrame& Stack,
        const FArgData& Data) const -> void override;

    virtual auto CanBeReturned(const FAngelscriptTypeUsage& Usage) const -> bool override;
    virtual auto GetReturnValue(
        const FAngelscriptTypeUsage& Usage,
        asIScriptContext* Context,
        void* Destination) const -> void override;

    virtual auto GetDebuggerValue(
        const FAngelscriptTypeUsage& Usage,
        void* Address,
        FDebuggerValue& Value) const -> bool override;

private:
    FString TypeName;
    FString ShortName;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Registry and factory for dynamic handle AngelScript types.
 */
class CKDYNAMIC_API FCkDynamic_AngelscriptTypeRegistry
{
public:
    /**
     * Initialize the type factory with the handle registry.
     * Called during module startup.
     */
    static auto
    Initialize() -> void;

    static auto
    RegisterType(
        const FString& InTypeName,
        const FString& InShortName) -> TSharedRef<FCkDynamic_AngelscriptType>;

    static auto
    GetType(
        const FString& InTypeName) -> TSharedPtr<FCkDynamic_AngelscriptType>;

    static auto
    IsTypeRegistered(
        const FString& InTypeName) -> bool;

private:
    static auto
    Get_RegisteredTypes() -> TMap<FString, TSharedRef<FCkDynamic_AngelscriptType>>&;

    static inline bool _Initialized = false;
};

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK