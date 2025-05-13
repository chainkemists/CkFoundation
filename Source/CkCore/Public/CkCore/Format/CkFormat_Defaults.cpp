#include "CkFormat_Defaults.h"

#include "ctti/type_id.hpp"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FName, [](const FName& InObj) { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FText, [](const FText& InObj) { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FGuid, [](const FGuid& InObj) { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FNetworkGUID, [](const FNetworkGUID& InObj) { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FKey, [](const FKey& InObj) { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FInputChord, [](const FInputChord& InObj) { return InObj.GetInputText().ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FSoftObjectPath, [](const FSoftObjectPath& InObj) { return InObj.GetAssetName(); });
CK_DEFINE_CUSTOM_FORMATTER(FSoftClassPath, [](const FSoftClassPath& InObj) { return InObj.GetAssetName(); });

CK_DEFINE_CUSTOM_FORMATTER(FRandomStream, [](const FRandomStream& InObj)
{
    return ck::Format_UE
    (
        TEXT("Initial Seed: [{}], Current Seed: [{}]"),
        InObj.GetInitialSeed(),
        InObj.GetCurrentSeed()
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FRotator, [](const FRotator& InObj)
{
    return ck::Format_UE
    (
        TEXT("P{:.2f}, Y:{:.2f}, R:{:.2f}"),
        InObj.Pitch,
        InObj.Yaw,
        InObj.Roll
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FVector, [](const FVector& InObj)
{
    return ck::Format_UE
    (
        TEXT("X:{:.2f}, Y:{:.2f}, Z:{:.2f}"),
        InObj.X,
        InObj.Y,
        InObj.Z
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FVector2D, [](const FVector2D& InObj)
{
    return ck::Format_UE
    (
        TEXT("X:{:.2f}, Y:{:.2f}"),
        InObj.X,
        InObj.Y
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FIntVector, [](const FIntVector& InObj)
{
    return ck::Format_UE
    (
        TEXT("X:{}, Y:{}, Z:{}"),
        InObj.X,
        InObj.Y,
        InObj.Z
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FIntVector2, [](const FIntVector2& InObj)
{
    return ck::Format_UE
    (
        TEXT("X:{}, Y:{}"),
        InObj.X,
        InObj.Y
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FIntVector4, [](const FIntVector4& InObj)
{
    return ck::Format_UE
    (
        TEXT("X:{}, Y:{}, Z:{}, W:{}"),
        InObj.X,
        InObj.Y,
        InObj.Z,
        InObj.W
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FQuat, [](const FQuat& InObj)
{
    return ck::Format_UE
    (
        TEXT("X:{:.2f}, Y:{:.2f}, Z:{:.2f}, W:{:.2f}"),
        InObj.X,
        InObj.Y,
        InObj.Z,
        InObj.W
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FBox, [](const FBox& InObj)
{
    return ck::Format_UE
    (
        TEXT("Min:{}, Max:{}, IsValid:{}"),
        InObj.Min,
        InObj.Max,
        InObj.IsValid
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FBox2D, [](const FBox2D& InObj)
{
    return ck::Format_UE
    (
        TEXT("Min:{}, Max:{}, IsValid:{}"),
        InObj.Min,
        InObj.Max,
        InObj.bIsValid
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FTransform, [](const FTransform& InObj)
{
    return ck::Format_UE
    (
        TEXT("T:[{}] R:[{}] S:[{}]"),
        InObj.GetTranslation(),
        InObj.GetRotation(),
        InObj.GetScale3D()
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FTimespan, [](const FTimespan& InObj)
{
    return ck::Format_UE
    (
        TEXT("Days:[{}] Hours:[{}] Minutes:[{}] Seconds:[{}]"),
        InObj.GetDays(),
        InObj.GetHours(),
        InObj.GetMinutes(),
        InObj.GetSeconds()
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FNativeGameplayTag, [](const FNativeGameplayTag& InObj)
{
    return ck::Format_UE(TEXT("{}"), InObj.GetTag());
});

#if CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION
CK_DEFINE_CUSTOM_FORMATTER(FGameplayTag, [](const FGameplayTag& InObj)
{
    if (ck::IsValid(InObj))
    {
        return ck::Format_UE(TEXT("{}"), InObj.ToString());
    }

    return ck::Format_UE(TEXT("TAG_NOT_SET"));
});
#else
CK_DEFINE_CUSTOM_FORMATTER(FGameplayTag, [](const FGameplayTag& InObj)
{
    if (ck::IsValid(InObj))
    {
        return ck::Format_UE(TEXT("{}"), InObj.ToString());
    }

    if (InObj.GetTagName() != NAME_None)
    {
        return ck::Format_UE(TEXT("{}[STALE]"), InObj.ToString());
    }

    return ck::Format_UE(TEXT("TAG_NOT_SET"));
});
#endif

CK_DEFINE_CUSTOM_FORMATTER(FGameplayTagContainer, [](const FGameplayTagContainer& InObj)
{
    if (ck::IsValid(InObj))
    {
        return ck::Format_UE(TEXT("{}"), InObj.ToString());
    }

    return ck::Format_UE(TEXT("TAG_CONTAINER_NOT_SET"));
});

CK_DEFINE_CUSTOM_FORMATTER(FAssetData, [](const FAssetData& InObj)
{
    return ck::Format_UE
    (
        TEXT("AssetPath: [{}]"),
        InObj.AssetClassPath.ToString()
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FCollisionProfileName, [](const FCollisionProfileName& InObj)
{
    return ck::Format_UE
    (
        TEXT("CollisionProfileName: [{}]"),
        InObj.Name
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FDataTableRowHandle, [](const FDataTableRowHandle& InObj)
{
    return ck::Format_UE
    (
        TEXT("DataTable: [{}], RowName: [{}]"),
        InObj.DataTable,
        InObj.RowName
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FCurveTableRowHandle, [](const FCurveTableRowHandle& InObj)
{
    return ck::Format_UE
    (
        TEXT("CurveTable: [{}], RowName: [{}]"),
        InObj.CurveTable,
        InObj.RowName
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FInstancedStruct, [](const FInstancedStruct& InObj)
{
    return ck::Format_UE(TEXT("{}"), InObj.GetScriptStruct());
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(SWindow, [](const SWindow& InObj)
{
    return ck::Format_UE(TEXT("{}"), InObj.ToString());
});

CK_DEFINE_CUSTOM_FORMATTER(UObject, [](const UObject& InObj)
{
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj).ToString();
});

CK_DEFINE_CUSTOM_FORMATTER(UActorComponent, [](const UActorComponent& InObj)
{
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj).ToString();
});

CK_DEFINE_CUSTOM_FORMATTER(AActor, [](const AActor& InObj)
{
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj).ToString();
});

CK_DEFINE_CUSTOM_FORMATTER(UClass, [](const UClass& InObj)
{
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj).ToString();
});

CK_DEFINE_CUSTOM_FORMATTER(UFunction, [](const UFunction& InObj)
{
    return ck::Format_UE(TEXT("{}.{}[{}]"), InObj.GetOuter()->GetName(), InObj.GetName(), InObj.GetUniqueID());
});

CK_DEFINE_CUSTOM_FORMATTER(FProperty, [](const FProperty& InObj)
{
    return ck::Format_UE(TEXT("{}[{}]"), InObj.GetName(), InObj.GetOffset_ForDebug());
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(ENetMode, [](const ENetMode& InObj)
{
    switch(InObj)
    {
        case NM_Standalone: return TEXT("Standalone");
        case NM_DedicatedServer: return TEXT("Dedicated Server");
        case NM_ListenServer: return TEXT("ListenServer");
        case NM_Client: return TEXT("Client");
        case NM_MAX: return TEXT("Max (INVALID)");
        default: return TEXT("Default (INVALID)");
    }
});

#if WITH_EDITOR
CK_DEFINE_CUSTOM_FORMATTER(EMapChangeType, [](const EMapChangeType& InObj)
{
    switch (InObj)
    {
        case EMapChangeType::NewMap: { return TEXT("NewMap"); }
        case EMapChangeType::LoadMap: { return TEXT("LoadMap"); }
        case EMapChangeType::TearDownWorld: { return TEXT("TearDownWorld"); }
        case EMapChangeType::SaveMap: { return TEXT("SaveMap"); }
        default:  { return TEXT("Unknown"); }
    }
});
#endif

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_NAMESPACE(ctti::detail::cstring, cstring, [](const ctti::detail::cstring& InObj)
{
    return FString(InObj.size(), InObj.begin());
});

// --------------------------------------------------------------------------------------------------------------------
