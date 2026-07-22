#include "CkDynamic/CkDynamic_Sentinel.h"

#include "CkCore/Validation/CkIsValid.h"

#include <UObject/GCObject.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_sentinel
{
    class FStore final : public FGCObject
    {
    public:
        auto Get(const UScriptStruct* InStructType) -> FInstancedStruct&
        {
            static auto InvalidNoType = FInstancedStruct{};
            if (ck::Is_NOT_Valid(InStructType))
            { return InvalidNoType; }

            auto& Instance = _Sentinels.FindOrAdd(InStructType);
            if (NOT Instance.IsValid() || Instance.GetScriptStruct() != InStructType)
            { Instance.InitializeAs(InStructType); }

            return Instance;
        }

        auto AddReferencedObjects(FReferenceCollector& InCollector) -> void override
        {
            for (auto& Pair : _Sentinels)
            { Pair.Value.AddStructReferencedObjects(InCollector); }
        }

        auto GetReferencerName() const -> FString override
        {
            return TEXT("CkDynamic invalid-fragment sentinel store");
        }

    private:
        // Weak keys avoid raw-UScriptStruct address ABA. Each value's AddStructReferencedObjects call intentionally
        // keeps its exact schema and any payload references alive for as long as script can retain the wildcard.
        TMap<TWeakObjectPtr<const UScriptStruct>, FInstancedStruct> _Sentinels;
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Get_InvalidSentinel_FragmentData(
        const UScriptStruct* InStructType)
    -> FInstancedStruct&
{
    static auto Store = ck_dynamic_sentinel::FStore{};
    return Store.Get(InStructType);
}

// --------------------------------------------------------------------------------------------------------------------
