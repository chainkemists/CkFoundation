#include "CkDynamic/CkDynamic_AngelScript.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Format/CkFormat.h"
#include "CkDynamic/CkDynamic_Utils.h"
#include "CkDynamic/CkDynamic_HandleDefinition.h"

#include <AngelscriptManager.h>
#include <AngelscriptType.h>
#include "AngelscriptCodeModule.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

#include "AngelscriptInclude.h"
#include "StartAngelscriptHeaders.h"
#include "as_scriptfunction.h"
#include "EndAngelscriptHeaders.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    Get_RegisteredTypes()
    -> TMap<FString, TSharedPtr<FCkDynamic_HandleTypeInfo>>&
{
    static TMap<FString, TSharedPtr<FCkDynamic_HandleTypeInfo>> RegisteredTypes;
    return RegisteredTypes;
}

auto
    FCkDynamic_HandleTypeRegistry::
    Get_PendingTypes()
    -> TArray<FCkDynamic_HandleTypeInfo>&
{
    static TArray<FCkDynamic_HandleTypeInfo> PendingTypes;
    return PendingTypes;
}

auto
    FCkDynamic_HandleTypeRegistry::
    Get_BoundConversionPairs()
    -> TSet<TPair<FString, FString>>&
{
    static TSet<TPair<FString, FString>> BoundPairs;
    return BoundPairs;
}

auto
    FCkDynamic_HandleTypeRegistry::
    GetAllRegisteredTypes()
    -> const TMap<FString, TSharedPtr<FCkDynamic_HandleTypeInfo>>&
{
    return Get_RegisteredTypes();
}

auto
    FCkDynamic_HandleTypeRegistry::
    GetRegistryFilePath()
    -> FString
{
    return FPaths::ProjectDir() / TEXT("Config") / TEXT("DynamicHandleTypes.json");
}

auto
    FCkDynamic_HandleTypeRegistry::
    ExtractShortName(
        const FString& InTypeName)
    -> FString
{
    static const FString Prefix = TEXT("Handle_");
    if (InTypeName.StartsWith(Prefix))
    {
        return InTypeName.RightChop(Prefix.Len());
    }
    return InTypeName;
}

auto
    FCkDynamic_HandleTypeRegistry::
    IsHandleTypeRegistered(
        const FString& InTypeName)
    -> bool
{
    return Get_RegisteredTypes().Contains(InTypeName);
}

auto
    FCkDynamic_HandleTypeRegistry::
    GetHandleTypeInfo(
        const FString& InTypeName)
    -> const FCkDynamic_HandleTypeInfo*
{
    const auto* Found = Get_RegisteredTypes().Find(InTypeName);
    if (Found == nullptr)
    {
        return nullptr;
    }
    return Found->Get();
}

auto
    FCkDynamic_HandleTypeRegistry::
    ValidateHandle(
        const FString& InTypeName,
        const FCk_Handle& InHandle)
    -> bool
{
    const auto* TypeInfo = GetHandleTypeInfo(InTypeName);
    if (TypeInfo == nullptr)
    {
        return false;
    }

    if (NOT TypeInfo->Validator)
    {
        return ck::IsValid(InHandle);
    }

    return TypeInfo->Validator(InHandle);
}

auto
    FCkDynamic_HandleTypeRegistry::
    FindScriptStructByName(
        const FString& InStructName)
    -> const UScriptStruct*
{
    auto* FoundStruct = FindObject<UScriptStruct>(nullptr, *InStructName);
    if (FoundStruct != nullptr)
    {
        return FoundStruct;
    }

    const auto AngelscriptType = FAngelscriptType::GetByAngelscriptTypeName(InStructName);
    if (AngelscriptType.IsValid())
    {
        const auto TypeUsage = FAngelscriptTypeUsage(AngelscriptType);
        const auto* UnrealStruct = TypeUsage.GetUnrealStruct();
        if (UnrealStruct != nullptr)
        {
            return Cast<UScriptStruct>(UnrealStruct);
        }
    }

    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        if (It->GetName() == InStructName)
        {
            return *It;
        }
    }

    return nullptr;
}

auto
    FCkDynamic_HandleTypeRegistry::
    CreateMultiFragmentValidator(
        const TArray<FString>& InFragmentNames)
    -> FHandleTypeValidator
{
    if (InFragmentNames.IsEmpty())
    {
        return nullptr;
    }

    return [FragmentNames = InFragmentNames](const FCk_Handle& InHandle) -> bool
        {
            if (NOT ck::IsValid(InHandle))
            {
                return false;
            }

            for (const auto& FragmentName : FragmentNames)
            {
                const auto* StructType = FCkDynamic_HandleTypeRegistry::FindScriptStructByName(FragmentName);
                if (StructType == nullptr)
                {
                    return false;
                }

                if (NOT UCk_Utils_DynamicFragment_UE::Has_Fragment(InHandle, StructType))
                {
                    return false;
                }
            }

            return true;
        };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    LoadFromJsonRegistry()
    -> bool
{
    if (_JsonRegistryLoaded)
    {
        return true;
    }

    const auto FilePath = GetRegistryFilePath();

    auto JsonString = FString{};
    if (NOT FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("[DynamicHandleTypes] No registry file found at: %s"), *FilePath);
        return false;
    }

    auto JsonReader = TJsonReaderFactory<>::Create(JsonString);
    auto RootObject = TSharedPtr<FJsonObject>{};

    if (NOT FJsonSerializer::Deserialize(JsonReader, RootObject) || NOT RootObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[DynamicHandleTypes] Failed to parse registry file: %s"), *FilePath);
        return false;
    }

    const auto HandleTypesArray = RootObject->GetArrayField(TEXT("HandleTypes"));
    if (HandleTypesArray.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[DynamicHandleTypes] Registry file is empty"));
        return true;
    }

    auto RegisteredCount = 0;

    for (const auto& HandleTypeValue : HandleTypesArray)
    {
        const auto HandleTypeObject = HandleTypeValue->AsObject();
        if (HandleTypeObject == nullptr)
        {
            continue;
        }

        auto TypeName = FString{};
        HandleTypeObject->TryGetStringField(TEXT("TypeName"), TypeName);

        if (TypeName.IsEmpty())
        {
            continue;
        }

        auto RequiredFragments = TArray<FString>{};
        const auto FragmentsArray = HandleTypeObject->GetArrayField(TEXT("RequiredFragments"));
        for (const auto& FragmentValue : FragmentsArray)
        {
            auto FragmentName = FString{};
            if (FragmentValue->TryGetString(FragmentName))
            {
                RequiredFragments.Add(FragmentName);
            }
        }

        auto Description = FString{};
        HandleTypeObject->TryGetStringField(TEXT("Description"), Description);

        auto SourceAsset = FString{};
        HandleTypeObject->TryGetStringField(TEXT("SourceAsset"), SourceAsset);

        if (RegisterHandleType(TypeName, RequiredFragments, Description, SourceAsset))
        {
            RegisteredCount++;
        }
    }

    _JsonRegistryLoaded = true;

    UE_LOG(LogTemp, Log, TEXT("[DynamicHandleTypes] Loaded %d handle types from registry"), RegisteredCount);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    EnsureCallbackRegistered()
    -> void
{
    static auto CallbackRegistered = false;
    if (CallbackRegistered)
    {
        return;
    }

    _PreCompileDelegateHandle = FAngelscriptCodeModule::GetPreCompile().AddStatic([]
        {
            LoadFromJsonRegistry();
            DiscoverAndRegisterAllDefinitions();
            RegisterAllPendingTypes();
            BindCrossHandleConversions();
            BindBaseMixinMethods();
        });

    CallbackRegistered = true;
}

auto
    FCkDynamic_HandleTypeRegistry::
    RegisterAllPendingTypes()
    -> void
{
    for (const auto& PendingInfo : Get_PendingTypes())
    {
        if (Get_RegisteredTypes().Contains(PendingInfo.TypeName))
        {
            continue;
        }

        auto TypeInfo = MakeShared<FCkDynamic_HandleTypeInfo>();
        TypeInfo->TypeName = PendingInfo.TypeName;
        TypeInfo->ShortName = PendingInfo.ShortName;
        TypeInfo->RequiredFragments = PendingInfo.RequiredFragments;
        TypeInfo->Description = PendingInfo.Description;
        TypeInfo->SourceAsset = PendingInfo.SourceAsset;
        TypeInfo->Validator = PendingInfo.Validator;

        Get_RegisteredTypes().Add(TypeInfo->TypeName, TypeInfo);
        CreateAngelScriptBindings(TypeInfo->TypeName);
    }

    Get_PendingTypes().Reset();
}

auto
    FCkDynamic_HandleTypeRegistry::
    DiscoverAndRegisterAllDefinitions()
    -> void
{
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    auto DefinitionAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssetsByClass(UCkDynamic_HandleDefinition::StaticClass()->GetClassPathName(), DefinitionAssets);

    for (const auto& AssetData : DefinitionAssets)
    {
        if (auto* Definition = Cast<UCkDynamic_HandleDefinition>(AssetData.GetAsset()))
        {
            RegisterHandleTypeFromDefinition(Definition);
        }
    }
}

auto
    FCkDynamic_HandleTypeRegistry::
    RegisterHandleTypeFromDefinition(
        const UCkDynamic_HandleDefinition* InDefinition)
    -> bool
{
    if (ck::Is_NOT_Valid(InDefinition))
    {
        return false;
    }

    if (InDefinition->TypeName.IsEmpty())
    {
        return false;
    }

    return RegisterHandleType(
        InDefinition->TypeName,
        InDefinition->GetRequiredFragmentNames(),
        InDefinition->Description,
        InDefinition->GetPathName());
}

auto
    FCkDynamic_HandleTypeRegistry::
    RegisterHandleType(
        const FString& InTypeName,
        const TArray<FString>& InRequiredFragments,
        const FString& InDescription,
        const FString& InSourceAsset)
    -> bool
{
    if (Get_RegisteredTypes().Contains(InTypeName))
    {
        return false;
    }

    for (const auto& Pending : Get_PendingTypes())
    {
        if (Pending.TypeName == InTypeName)
        {
            return false;
        }
    }

    auto TypeInfo = FCkDynamic_HandleTypeInfo{};
    TypeInfo.TypeName = InTypeName;
    TypeInfo.ShortName = ExtractShortName(InTypeName);
    TypeInfo.RequiredFragments = InRequiredFragments;
    TypeInfo.Description = InDescription;
    TypeInfo.SourceAsset = InSourceAsset;
    TypeInfo.Validator = CreateMultiFragmentValidator(InRequiredFragments);

    if (FAngelscriptManager::IsInitialized())
    {
        auto SharedInfo = MakeShared<FCkDynamic_HandleTypeInfo>(MoveTemp(TypeInfo));
        Get_RegisteredTypes().Add(InTypeName, SharedInfo);
        CreateAngelScriptBindings(InTypeName);
        return true;
    }

    Get_PendingTypes().Add(MoveTemp(TypeInfo));
    EnsureCallbackRegistered();
    return true;
}

auto
    FCkDynamic_HandleTypeRegistry::
    RegisterHandleType(
        const FString& InTypeName,
        const FString& InValidatorFragmentName)
    -> bool
{
    auto Fragments = TArray<FString>{};
    if (NOT InValidatorFragmentName.IsEmpty())
    {
        Fragments.Add(InValidatorFragmentName);
    }
    return RegisterHandleType(InTypeName, Fragments);
}

auto
    FCkDynamic_HandleTypeRegistry::
    RegisterHandleTypeWithValidator(
        const FString& InTypeName,
        const FHandleTypeValidator& InValidator)
    -> bool
{
    if (Get_RegisteredTypes().Contains(InTypeName))
    {
        return false;
    }

    for (const auto& Pending : Get_PendingTypes())
    {
        if (Pending.TypeName == InTypeName)
        {
            return false;
        }
    }

    auto TypeInfo = FCkDynamic_HandleTypeInfo{};
    TypeInfo.TypeName = InTypeName;
    TypeInfo.ShortName = ExtractShortName(InTypeName);
    TypeInfo.Validator = InValidator;

    if (FAngelscriptManager::IsInitialized())
    {
        auto SharedInfo = MakeShared<FCkDynamic_HandleTypeInfo>(MoveTemp(TypeInfo));
        Get_RegisteredTypes().Add(InTypeName, SharedInfo);
        CreateAngelScriptBindings(InTypeName);
        return true;
    }

    Get_PendingTypes().Add(MoveTemp(TypeInfo));
    EnsureCallbackRegistered();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GetTypeInfoFromGeneric(asIScriptGeneric* InGeneric) -> FCkDynamic_HandleTypeInfo*
    {
        if (InGeneric == nullptr)
        {
            return nullptr;
        }

        auto* Function = static_cast<asCScriptFunction*>(InGeneric->GetFunction());
        if (Function == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<FCkDynamic_HandleTypeInfo*>(Function->userData);
    }

    auto ValidateHandleWithTypeInfo(const FCk_Handle& InHandle, FCkDynamic_HandleTypeInfo* InTypeInfo) -> bool
    {
        if (InTypeInfo == nullptr)
        {
            return ck::IsValid(InHandle);
        }

        if (NOT InTypeInfo->Validator)
        {
            return ck::IsValid(InHandle);
        }

        return InTypeInfo->Validator(InHandle);
    }

    void SetPreviousFunctionUserData(void* InUserData)
    {
        const auto FunctionId = FAngelscriptBinds::GetPreviousFunctionId();
        auto* ScriptFunction = static_cast<asCScriptFunction*>(
            FAngelscriptManager::Get().Engine->GetFunctionById(FunctionId));

        if (ScriptFunction != nullptr)
        {
            ScriptFunction->userData = InUserData;
        }
    }

    auto TriggerDynamicHandleCastEnsure(
        const FCk_Handle& InHandle,
        const FString& InTargetTypeName) -> void
    {
        const auto& Message = ck::Format_UE(
            TEXT("Handle [{}] does NOT have required fragments for [{}]. Unable to convert Handle."),
            InHandle,
            InTargetTypeName);

        UCk_Utils_Ensure_UE::TriggerEnsure(FText::FromString(Message), nullptr);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    CreateAngelScriptBindings(
        const FString& InTypeName)
    -> void
{
    const auto* TypeInfoPtr = Get_RegisteredTypes().Find(InTypeName);
    if (TypeInfoPtr == nullptr)
    {
        return;
    }

    const auto& TypeInfo = *TypeInfoPtr;
    const auto& TypeName = TypeInfo->TypeName;
    const auto& ShortName = TypeInfo->ShortName;
    const auto TypeNameAnsi = StringCast<ANSICHAR>(*TypeName);
    const auto TypeNameStr = TypeNameAnsi.Get();

    auto* UserData = TypeInfo.Get();

    auto Bind = FAngelscriptBinds::ValueClass(TypeNameStr, sizeof(FCk_Handle), FBindFlags());

    // ------------------------------------------------
    // Constructors
    // ------------------------------------------------

    Bind.Constructor("void f()", [](FCk_Handle* Address)
        {
            new(Address) FCk_Handle();
        });

    auto CopyCtorSig = ck::Format_ANSI(TEXT("void f(const {}& in InOther)"), TypeName);
    Bind.Constructor(CopyCtorSig.c_str(), [](FCk_Handle* Address, const FCk_Handle& InOther)
        {
            new(Address) FCk_Handle(InOther);
        });

    Bind.Constructor("void f(const FCk_Handle& in InHandle)",
        [](FCk_Handle* Address, const FCk_Handle& InHandle)
        {
            new(Address) FCk_Handle(InHandle);
        });

    // ------------------------------------------------
    // Destructor
    // ------------------------------------------------

    Bind.Destructor("void f()", [](FCk_Handle* Address)
        {
            Address->~FCk_Handle();
        });

    // ------------------------------------------------
    // Assignment Operators
    // ------------------------------------------------

    auto AssignSelfSig = ck::Format_ANSI(TEXT("{}& opAssign(const {}& in InOther)"), TypeName, TypeName);
    Bind.Method(AssignSelfSig.c_str(), [](FCk_Handle& Self, const FCk_Handle& InOther) -> FCk_Handle&
        {
            Self = InOther;
            return Self;
        });

    auto AssignBaseSig = ck::Format_ANSI(TEXT("{}& opAssign(const FCk_Handle& in InHandle)"), TypeName);
    Bind.Method(AssignBaseSig.c_str(), [](FCk_Handle& Self, const FCk_Handle& InHandle) -> FCk_Handle&
        {
            Self = InHandle;
            return Self;
        });

    // ------------------------------------------------
    // Implicit Conversions
    // ------------------------------------------------

    Bind.Method("FCk_Handle opImplConv() const", [](const FCk_Handle& Self) -> FCk_Handle
        {
            return Self;
        });

    Bind.Method("FCk_Handle& opImplCast()", [](FCk_Handle& Self) -> FCk_Handle&
        {
            return Self;
        });

    Bind.Method("const FCk_Handle& opImplCast() const", [](const FCk_Handle& Self) -> const FCk_Handle&
        {
            return Self;
        });

    // ------------------------------------------------
    // Handle Accessors
    // ------------------------------------------------

    Bind.Method("FCk_Handle& H()", [](FCk_Handle& Self) -> FCk_Handle&
        {
            return Self;
        });

    Bind.Method("const FCk_Handle& H() const", [](const FCk_Handle& Self) -> const FCk_Handle&
        {
            return Self;
        });

    // ------------------------------------------------
    // Validation - IsValid() validates type (fragments)
    // ------------------------------------------------

    Bind.GenericMethod("bool IsValid() const",
        [](asIScriptGeneric* InGeneric)
        {
            auto* Self = static_cast<FCk_Handle*>(InGeneric->GetObject());
            auto* TypeInfo = GetTypeInfoFromGeneric(InGeneric);
            const auto Result = ValidateHandleWithTypeInfo(*Self, TypeInfo);
            InGeneric->SetReturnByte(Result ? 1 : 0);
        }, nullptr);
    SetPreviousFunctionUserData(UserData);

    // ------------------------------------------------
    // Utility Methods
    // ------------------------------------------------

    Bind.Method("FString ToString() const", [](const FCk_Handle& Self) -> FString
        {
            return Self.ToString();
        });

    Bind.Method("FString Debug() const", [](const FCk_Handle& Self) -> FString
        {
            Self.DoFireEnsure();
            return Self.ToString();
        });

    // ------------------------------------------------
    // Equality Operators
    // ------------------------------------------------

    auto EqualsSelfSig = ck::Format_ANSI(TEXT("bool opEquals(const {}& in Other) const"), TypeName);
    Bind.Method(EqualsSelfSig.c_str(), [](const FCk_Handle& A, const FCk_Handle& B) -> bool
        {
            return A == B;
        });

    Bind.Method("bool opEquals(const FCk_Handle& in Other) const",
        [](const FCk_Handle& A, const FCk_Handle& B) -> bool
        {
            return A == B;
        });

    // ------------------------------------------------
    // Base Handle Extensions (using short name)
    // ------------------------------------------------

    auto BaseBind = FAngelscriptBinds::ExistingClass("FCk_Handle");
    if (BaseBind.GetTypeInfo() == nullptr)
    {
        return;
    }

    auto AsMethodSig = ck::Format_ANSI(
        TEXT("{} As_{}(ECk_SanityCheck InChecked = ECk_SanityCheck::Checked) const"),
        TypeName,
        ShortName);

    BaseBind.GenericMethod(AsMethodSig.c_str(),
        [](asIScriptGeneric* InGeneric)
        {
            auto* Self = static_cast<const FCk_Handle*>(InGeneric->GetObject());
            auto Checked = *static_cast<ECk_SanityCheck*>(InGeneric->GetAddressOfArg(0));
            auto* TypeInfo = GetTypeInfoFromGeneric(InGeneric);

            auto Result = FCk_Handle{};
            const auto IsValidAsType = ValidateHandleWithTypeInfo(*Self, TypeInfo);

            if (IsValidAsType)
            {
                Result = *Self;
            }
            else if (Checked == ECk_SanityCheck::Checked)
            {
                const auto TargetTypeName = TypeInfo != nullptr ? TypeInfo->TypeName : TEXT("Unknown");
                TriggerDynamicHandleCastEnsure(*Self, TargetTypeName);
            }

            new(InGeneric->GetAddressOfReturnLocation()) FCk_Handle(Result);
        }, nullptr);
    SetPreviousFunctionUserData(UserData);

    auto IsMethodSig = ck::Format_ANSI(TEXT("bool Is_{}() const"), ShortName);
    BaseBind.GenericMethod(IsMethodSig.c_str(),
        [](asIScriptGeneric* InGeneric)
        {
            auto* Self = static_cast<const FCk_Handle*>(InGeneric->GetObject());
            auto* TypeInfo = GetTypeInfoFromGeneric(InGeneric);
            const auto Result = ValidateHandleWithTypeInfo(*Self, TypeInfo);
            InGeneric->SetReturnByte(Result ? 1 : 0);
        }, nullptr);
    SetPreviousFunctionUserData(UserData);

    auto BaseEqualsSig = ck::Format_ANSI(TEXT("bool opEquals(const {}& in Other) const"), TypeName);
    BaseBind.Method(BaseEqualsSig.c_str(), [](const FCk_Handle& A, const FCk_Handle& B) -> bool
        {
            return A == B;
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    BindCrossHandleConversions()
    -> void
{
    if (_CrossConversionsBound)
    {
        return;
    }
    _CrossConversionsBound = true;

    const auto& Types = Get_RegisteredTypes();
    auto& BoundPairs = Get_BoundConversionPairs();

    struct FAsMethodAuxData
    {
        FHandleTypeValidator Validator;
        FString TypeName;
    };

    struct FIsMethodAuxData
    {
        FHandleTypeValidator Validator;
    };

    static TMap<asIScriptFunction*, FAsMethodAuxData> AsMethodAuxDataMap;
    static TMap<asIScriptFunction*, FIsMethodAuxData> IsMethodAuxDataMap;

    for (const auto& SourcePair : Types)
    {
        const auto& SourceType = SourcePair.Value;
        auto SourceBind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*SourceType->TypeName));
        if (SourceBind.GetTypeInfo() == nullptr)
        {
            continue;
        }

        for (const auto& TargetPair : Types)
        {
            const auto& TargetType = TargetPair.Value;

            if (SourceType->TypeName == TargetType->TypeName)
            {
                continue;
            }

            auto PairKey = TPair<FString, FString>{ SourceType->TypeName, TargetType->TypeName };
            if (BoundPairs.Contains(PairKey))
            {
                continue;
            }

            auto AsMethodSig = ck::Format_ANSI(
                TEXT("{} As_{}(ECk_SanityCheck InChecked = ECk_SanityCheck::Checked) const"),
                TargetType->TypeName,
                TargetType->ShortName);

            auto AsAuxData = FAsMethodAuxData{};
            AsAuxData.Validator = TargetType->Validator;
            AsAuxData.TypeName = TargetType->TypeName;

            SourceBind.GenericMethod(AsMethodSig.c_str(),
                [](asIScriptGeneric* InGeneric)
                {
                    auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                    auto Checked = *static_cast<ECk_SanityCheck*>(InGeneric->GetAddressOfArg(0));
                    auto* Function = InGeneric->GetFunction();
                    auto* AuxData = AsMethodAuxDataMap.Find(Function);

                    if (AuxData == nullptr)
                    {
                        UE_LOG(LogTemp, Error, TEXT("[As_] AuxData lookup failed!"));
                        auto* ReturnLocation = InGeneric->GetAddressOfReturnLocation();
                        FMemory::Memzero(ReturnLocation, sizeof(FCk_Handle));
                        return;
                    }

                    auto IsValidAsType = false;
                    if (AuxData->Validator)
                    {
                        IsValidAsType = AuxData->Validator(*Handle);
                    }
                    else
                    {
                        IsValidAsType = ck::IsValid(*Handle);
                    }

                    auto Result = FCk_Handle{};
                    if (IsValidAsType)
                    {
                        Result = *Handle;
                    }
                    else if (Checked == ECk_SanityCheck::Checked)
                    {
                        TriggerDynamicHandleCastEnsure(*Handle, AuxData->TypeName);
                    }

                    new(InGeneric->GetAddressOfReturnLocation()) FCk_Handle(Result);
                }, nullptr);

            auto* TypeInfo = SourceBind.GetTypeInfo();
            if (TypeInfo != nullptr)
            {
                auto AsMethodKey = ck::Format_ANSI(TEXT("As_{}"), TargetType->ShortName);
                auto* RegisteredFunc = TypeInfo->GetMethodByName(AsMethodKey.c_str());
                if (RegisteredFunc != nullptr)
                {
                    AsMethodAuxDataMap.Add(RegisteredFunc, AsAuxData);
                }
            }

            auto IsMethodSig = ck::Format_ANSI(TEXT("bool Is_{}() const"), TargetType->ShortName);

            auto IsAuxData = FIsMethodAuxData{};
            IsAuxData.Validator = TargetType->Validator;

            SourceBind.GenericMethod(IsMethodSig.c_str(),
                [](asIScriptGeneric* InGeneric)
                {
                    auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                    auto* Function = InGeneric->GetFunction();
                    auto* AuxData = IsMethodAuxDataMap.Find(Function);

                    auto Result = false;
                    if (AuxData != nullptr && AuxData->Validator)
                    {
                        Result = AuxData->Validator(*Handle);
                    }
                    else if (AuxData != nullptr)
                    {
                        Result = ck::IsValid(*Handle);
                    }

                    InGeneric->SetReturnByte(Result ? 1 : 0);
                }, nullptr);

            if (TypeInfo != nullptr)
            {
                auto IsMethodKey = ck::Format_ANSI(TEXT("Is_{}"), TargetType->ShortName);
                auto* RegisteredFunc = TypeInfo->GetMethodByName(IsMethodKey.c_str());
                if (RegisteredFunc != nullptr)
                {
                    IsMethodAuxDataMap.Add(RegisteredFunc, IsAuxData);
                }
            }

            BoundPairs.Add(PairKey);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    BindBaseMixinMethods()
    -> void
{
    if (_BaseMixinsBound)
    {
        return;
    }
    _BaseMixinsBound = true;

    auto* Engine = FAngelscriptManager::Get().GetScriptEngine();
    if (Engine == nullptr)
    {
        return;
    }

    auto* BaseTypeInfo = Engine->GetTypeInfoByName("FCk_Handle");
    if (BaseTypeInfo == nullptr)
    {
        return;
    }

    const auto& DerivedTypes = Get_RegisteredTypes();

    struct FMethodInfo
    {
        FString Name;
        FString Declaration;
        asIScriptFunction* Function;
    };
    TArray<FMethodInfo> MethodsToBind;

    auto MethodCount = BaseTypeInfo->GetMethodCount();
    for (asUINT MethodIndex = 0; MethodIndex < MethodCount; ++MethodIndex)
    {
        auto* Method = BaseTypeInfo->GetMethodByIndex(MethodIndex);
        if (Method == nullptr)
        {
            continue;
        }

        if (Method->GetFuncType() != asFUNC_SYSTEM)
        {
            continue;
        }

        auto MethodName = FString(Method->GetName());

        if (MethodName.StartsWith(TEXT("op")) ||
            MethodName.StartsWith(TEXT("As_")) ||
            MethodName.StartsWith(TEXT("Is_")) ||
            MethodName == TEXT("IsValid") ||
            MethodName == TEXT("ToString") ||
            MethodName == TEXT("Debug") ||
            MethodName == TEXT("H"))
        {
            continue;
        }

        auto Declaration = FString(Method->GetDeclaration(false, false, true, false));
        MethodsToBind.Add(FMethodInfo{ MethodName, Declaration, Method });
    }

    static TMap<asIScriptFunction*, asIScriptFunction*> BaseMixinMethodMap;

    for (const auto& DerivedPair : DerivedTypes)
    {
        const auto& DerivedType = DerivedPair.Value;
        auto DerivedBind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*DerivedType->TypeName));
        if (DerivedBind.GetTypeInfo() == nullptr)
        {
            continue;
        }

        for (const auto& MethodInfo : MethodsToBind)
        {
            DerivedBind.GenericMethod(TCHAR_TO_ANSI(*MethodInfo.Declaration),
                [](asIScriptGeneric* InGeneric)
                {
                    auto* DerivedFunc = InGeneric->GetFunction();
                    auto* OriginalMethodPtr = BaseMixinMethodMap.Find(DerivedFunc);

                    if (OriginalMethodPtr == nullptr || *OriginalMethodPtr == nullptr)
                    {
                        return;
                    }

                    auto* OriginalMethod = *OriginalMethodPtr;
                    auto* Engine = InGeneric->GetEngine();
                    auto* Context = Engine->RequestContext();
                    if (Context == nullptr)
                    {
                        return;
                    }

                    Context->Prepare(OriginalMethod);
                    Context->SetObject(InGeneric->GetObject());

                    auto ArgCount = static_cast<asUINT>(InGeneric->GetArgCount());
                    for (asUINT ArgIdx = 0; ArgIdx < ArgCount; ++ArgIdx)
                    {
                        asDWORD Flags = 0;
                        auto TypeId = InGeneric->GetArgTypeId(ArgIdx, &Flags);

                        if (TypeId == asTYPEID_BOOL || TypeId == asTYPEID_INT8 || TypeId == asTYPEID_UINT8)
                        {
                            Context->SetArgByte(ArgIdx, InGeneric->GetArgByte(ArgIdx));
                        }
                        else if (TypeId == asTYPEID_INT16 || TypeId == asTYPEID_UINT16)
                        {
                            Context->SetArgWord(ArgIdx, InGeneric->GetArgWord(ArgIdx));
                        }
                        else if (TypeId == asTYPEID_INT32 || TypeId == asTYPEID_UINT32 || TypeId == asTYPEID_FLOAT32)
                        {
                            Context->SetArgDWord(ArgIdx, InGeneric->GetArgDWord(ArgIdx));
                        }
                        else if (TypeId == asTYPEID_INT64 || TypeId == asTYPEID_UINT64 || TypeId == asTYPEID_FLOAT64)
                        {
                            Context->SetArgQWord(ArgIdx, InGeneric->GetArgQWord(ArgIdx));
                        }
                        else
                        {
                            Context->SetArgAddress(ArgIdx, InGeneric->GetAddressOfArg(ArgIdx));
                        }
                    }

                    Context->Execute();

                    asDWORD RetFlags = 0;
                    auto RetTypeId = InGeneric->GetReturnTypeId(&RetFlags);
                    if (RetTypeId != asTYPEID_VOID)
                    {
                        if (RetTypeId == asTYPEID_BOOL || RetTypeId == asTYPEID_INT8 || RetTypeId == asTYPEID_UINT8)
                        {
                            InGeneric->SetReturnByte(Context->GetReturnByte());
                        }
                        else if (RetTypeId == asTYPEID_INT16 || RetTypeId == asTYPEID_UINT16)
                        {
                            InGeneric->SetReturnWord(Context->GetReturnWord());
                        }
                        else if (RetTypeId == asTYPEID_INT32 || RetTypeId == asTYPEID_UINT32 || RetTypeId == asTYPEID_FLOAT32)
                        {
                            InGeneric->SetReturnDWord(Context->GetReturnDWord());
                        }
                        else if (RetTypeId == asTYPEID_INT64 || RetTypeId == asTYPEID_UINT64 || RetTypeId == asTYPEID_FLOAT64)
                        {
                            InGeneric->SetReturnQWord(Context->GetReturnQWord());
                        }
                        else
                        {
                            auto* SrcAddress = Context->GetReturnAddress();
                            auto* DstAddress = InGeneric->GetAddressOfReturnLocation();
                            if (SrcAddress != nullptr && DstAddress != nullptr)
                            {
                                new (DstAddress) FCk_Handle(*static_cast<const FCk_Handle*>(SrcAddress));
                            }
                        }
                    }

                    Engine->ReturnContext(Context);
                }, nullptr);

            auto* TypeInfo = DerivedBind.GetTypeInfo();
            if (TypeInfo != nullptr)
            {
                auto* RegisteredFunc = TypeInfo->GetMethodByName(TCHAR_TO_ANSI(*MethodInfo.Name));
                if (RegisteredFunc != nullptr)
                {
                    BaseMixinMethodMap.Add(RegisteredFunc, MethodInfo.Function);
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_DynamicHandleTypes_Init(
    FAngelscriptBinds::EOrder::Late, []
    {
        FCkDynamic_HandleTypeRegistry::EnsureCallbackRegistered();
    });

// --------------------------------------------------------------------------------------------------------------------

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_RegisterHandleType_Global(
    FAngelscriptBinds::EOrder::Late, []
    {
        FAngelscriptBinds::BindGlobalFunction(
            "bool Ck_RegisterHandleType(const FString& in InTypeName)",
            [](const FString& InTypeName) -> bool
            {
                return FCkDynamic_HandleTypeRegistry::RegisterHandleType(InTypeName);
            });

        FAngelscriptBinds::BindGlobalFunction(
            "bool Ck_RegisterHandleTypeWithFragment(const FString& in InTypeName, const FString& in InValidatorFragment)",
            [](const FString& InTypeName, const FString& InValidatorFragment) -> bool
            {
                return FCkDynamic_HandleTypeRegistry::RegisterHandleType(InTypeName, TArray<FString>{InValidatorFragment});
            });

        FAngelscriptBinds::BindGlobalFunction(
            "bool Ck_IsHandleTypeRegistered(const FString& in InTypeName)",
            [](const FString& InTypeName) -> bool
            {
                return FCkDynamic_HandleTypeRegistry::IsHandleTypeRegistered(InTypeName);
            });
    });

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------