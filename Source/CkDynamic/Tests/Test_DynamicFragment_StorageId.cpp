// Tests for UCk_Utils_DynamicFragment_UE::Get_StorageId (per-UScriptStruct StorageId cache).
//
// Coverage: cache identity (same struct -> same id), stability against the legacy
// GetTypeHash(GetPathName()) computation, and distinctness across different structs.

#include "CkDynamic/CkDynamic_Utils.h"
#include "CkDynamic/CkDynamic_Fragment_Data.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DynamicFragment_StorageIdCache,
    "Ck.CkDynamic.StorageId.CacheIdentityAndStability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DynamicFragment_StorageIdCache::RunTest(const FString&)
{
    const auto* StructA = FCk_Fragment_DynamicFragment_Data::StaticStruct();
    const auto* StructB = FCk_DynamicFragment_RepNotifyInfo::StaticStruct();

    const auto IdA1 = UCk_Utils_DynamicFragment_UE::Get_StorageId(StructA);
    const auto IdA2 = UCk_Utils_DynamicFragment_UE::Get_StorageId(StructA);
    const auto IdB  = UCk_Utils_DynamicFragment_UE::Get_StorageId(StructB);

    TestEqual(TEXT("same struct -> same id"), IdA1, IdA2);
    TestEqual(TEXT("cache matches legacy computation"),
        IdA1, entt::id_type{GetTypeHash(StructA->GetPathName())});
    TestNotEqual(TEXT("different structs -> different ids"), IdA1, IdB);
    return true;
}

#endif
