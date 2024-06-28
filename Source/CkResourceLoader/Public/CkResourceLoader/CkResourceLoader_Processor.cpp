#include "CkResourceLoader_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkResourceLoader/CkResourceLoader_Log.h"
#include "CkResourceLoader/CkResourceLoader_Utils.h"
#include "CkResourceLoader/Subsystem/CkResourceLoader_Subsystem.h"
#include "UObject/ObjectSaveContext.h"

#include <Engine/AssetManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ResourceLoader_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ResourceLoader_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResourceLoader_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InRequest);
        }), policy::DontResetContainer{});

        if (InRequestsComp.Get_Requests().IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle, ECk_EntityLifetime_DestructionBehavior::DestroyOnlyIfOrphan);
        }
    }

    auto
        FProcessor_ResourceLoader_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ResourceLoader_LoadObject& InRequest) const
        -> void
    {
        const auto& ObjectToLoadSoftRef = InRequest.Get_ObjectReference_Soft();

        CK_ENSURE_IF_NOT(ck::IsValid(ObjectToLoadSoftRef), TEXT("Invalid Object to Load!"))
        { return; }

        const auto& ObjectToLoadPath = ObjectToLoadSoftRef.Get_SoftObjectPath();
        const auto& LoadingPolicy    = InRequest.Get_LoadingPolicy();

        resource_loader::VeryVerbose
        (
            TEXT("Handling Request Load Object for [{}] with Loading Policy [{}]"),
            ObjectToLoadPath,
            LoadingPolicy
        );

        switch (LoadingPolicy)
        {
            case ECk_ResourceLoader_LoadingPolicy::Async:
            {
                auto PendingObject = FCk_ResourceLoader_PendingObject{ObjectToLoadSoftRef};

                const auto& StreamingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad
                (
                    ObjectToLoadPath,
                    FStreamableDelegate::CreateRaw(this, &ThisType::DoOnPendingObjectStreamed, InHandle ,ObjectToLoadSoftRef)
                );

                CK_ENSURE_IF_NOT(ck::IsValid(StreamingHandle),
                    TEXT("Failed to RequestAsyncLoad for [{}] with Loading Policy [{}]!"),
                    ObjectToLoadSoftRef,
                    LoadingPolicy)
                { return; }

                PendingObject.Set_StreamableHandle(StreamingHandle);

                UCk_Utils_ResourceLoader_UE::DoAddPendingObject(InHandle, PendingObject);

                break;
            }
            case ECk_ResourceLoader_LoadingPolicy::Synchronous:
            {
                const auto& LoadedObject = ObjectToLoadPath.TryLoad();

                const auto ObjectToLoadHardRef = FCk_ResourceLoader_ObjectReference_Hard{LoadedObject};

                DoOnObjectLoaded(InHandle, FCk_ResourceLoader_LoadedObject{ObjectToLoadSoftRef, ObjectToLoadHardRef});

                break;
            }
            default:
            {
                CK_INVALID_ENUM(LoadingPolicy);
                break;
            }
        }
    }

    auto
        FProcessor_ResourceLoader_HandleRequests::
        DoHandleRequest(
            FCk_Handle InHandle,
            const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest) const
        -> void
    {
        const auto& ObjectToLoadSoftRefs = InRequest.Get_ObjectReferences_Soft();
        const auto& LoadingPolicy        = InRequest.Get_LoadingPolicy();

        resource_loader::VeryVerbose
        (
            TEXT("Handling Request Load Object Batch with Loading Policy [{}]"),
            LoadingPolicy
        );

        switch (LoadingPolicy)
        {
            case ECk_ResourceLoader_LoadingPolicy::Async:
            {
                auto PendingObjectBatch = FCk_ResourceLoader_PendingObjectBatch{ObjectToLoadSoftRefs};

                const auto ObjectToLoadPaths = algo::Transform<TArray<FSoftObjectPath>>(ObjectToLoadSoftRefs,
                [](const FCk_ResourceLoader_ObjectReference_Soft& InObjectRefSoft)
                {
                    return InObjectRefSoft.Get_SoftObjectPath();
                });

                if (ObjectToLoadPaths.IsEmpty())
                { break; }

                const auto& StreamingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad
                (
                    ObjectToLoadPaths,
                    FStreamableDelegate::CreateRaw(this, &ThisType::DoOnPendingObjectBatchStreamed, InHandle, ObjectToLoadSoftRefs)
                );

                CK_ENSURE_IF_NOT(ck::IsValid(StreamingHandle),
                    TEXT("Failed to RequestAsyncLoad  with Loading Policy [{}]!"),
                    LoadingPolicy)
                { return; }

                PendingObjectBatch.Set_StreamableHandle(StreamingHandle);

                UCk_Utils_ResourceLoader_UE::DoAddPendingObjectBatch(InHandle, PendingObjectBatch);

                break;
            }
            case ECk_ResourceLoader_LoadingPolicy::Synchronous:
            {
                const auto& LoadedObjects = algo::Transform<TArray<FCk_ResourceLoader_LoadedObject>>(ObjectToLoadSoftRefs,
                [&](const FCk_ResourceLoader_ObjectReference_Soft& InObjectRefSoft)
                {
                    const auto& ObjectToLoadPath   = InObjectRefSoft.Get_SoftObjectPath();
                    const auto ObjectToLoadHardRef = FCk_ResourceLoader_ObjectReference_Hard{ObjectToLoadPath.TryLoad()};
                    return FCk_ResourceLoader_LoadedObject{InObjectRefSoft, ObjectToLoadHardRef};
                });

                const auto UniqueLoadedObjects = TSet(LoadedObjects).Array();
                    
                DoOnObjectBatchLoaded(InHandle, FCk_ResourceLoader_LoadedObjectBatch{UniqueLoadedObjects, LoadedObjects});

                break;
            }
            default:
            {
                CK_INVALID_ENUM(LoadingPolicy);
                break;
            }
        }
    }

    auto
        FProcessor_ResourceLoader_HandleRequests::
        DoOnPendingObjectStreamed(
            HandleType InHandle,
            FCk_ResourceLoader_ObjectReference_Soft InObjectStreamed) const
        -> void
    {
        const auto& AllPendingObjects = InHandle.Get<FFragment_ResourceLoader_PendingObjects>().Get_PendingObjects();
        const auto& FoundPendingObjectIndex = AllPendingObjects.Find(FCk_ResourceLoader_PendingObject{InObjectStreamed});

        CK_ENSURE_IF_NOT(FoundPendingObjectIndex != INDEX_NONE,
            TEXT("Could not find Streamed Object [{}] in the list of Pending Object!"),
            InObjectStreamed)
        { return; }

        const auto& FoundPendingObject = AllPendingObjects[FoundPendingObjectIndex];
        const auto& StreamingHandle    = FoundPendingObject.Get_StreamableHandle();
        const auto& LoadedAsset        = StreamingHandle->GetLoadedAsset();
        const auto ObjectToLoadHardRef = FCk_ResourceLoader_ObjectReference_Hard{LoadedAsset};
        const auto LoadedObject        = FCk_ResourceLoader_LoadedObject{InObjectStreamed, ObjectToLoadHardRef}.Set_StreamableHandle(StreamingHandle);

        DoOnObjectLoaded(InHandle, LoadedObject);
    }

    auto
        FProcessor_ResourceLoader_HandleRequests::
        DoOnPendingObjectBatchStreamed(
            HandleType InHandle,
            // ReSharper disable once CppPassValueParameterByConstReference
            TArray<FCk_ResourceLoader_ObjectReference_Soft> InObjectBatchStreamed) const
        -> void
    {
        const auto& AllPendingObjectBatches = InHandle.Get<FFragment_ResourceLoader_PendingObjectBatches>().Get_PendingObjectBatches();
        const auto& FoundPendingObjectBatchIndex = AllPendingObjectBatches.Find(FCk_ResourceLoader_PendingObjectBatch{InObjectBatchStreamed});

        CK_ENSURE_IF_NOT(FoundPendingObjectBatchIndex != INDEX_NONE,
            TEXT("Could not find Streamed Object Batch in the list of Pending Object Batch"))
        { return; }

        const auto& FoundPendingObjectBatch = AllPendingObjectBatches[FoundPendingObjectBatchIndex];
        const auto& StreamingHandle         = FoundPendingObjectBatch.Get_StreamableHandle();

        auto LoadedAssets = TArray<UObject*>{};
        StreamingHandle->GetLoadedAssets(LoadedAssets);

        auto PathToLoadedAssetsMap = TMap<FSoftObjectPath,UObject*>{};
        for (const auto& LoadedAsset : LoadedAssets)
        {
            PathToLoadedAssetsMap.Add(FSoftObjectPath(LoadedAsset), LoadedAsset);
        }
        
        const auto& LoadedObjects = algo::Transform<TArray<FCk_ResourceLoader_LoadedObject>>(InObjectBatchStreamed,
        [&](const FCk_ResourceLoader_ObjectReference_Soft& LoadedObjectSoftRef)
            -> FCk_ResourceLoader_LoadedObject
        {
            FSoftObjectPath SoftObjectPath = LoadedObjectSoftRef.Get_SoftObjectPath();
            const auto FoundLoadedAsset = PathToLoadedAssetsMap.Find(SoftObjectPath);

            CK_ENSURE_IF_NOT(ck::IsValid(FoundLoadedAsset, ck::IsValid_Policy_NullptrOnly{}),
                TEXT("Failed to load asset [{}]!"),
                SoftObjectPath.GetAssetPathString())
            { return {}; }
            
            const auto LoadedObjectHardRef = FCk_ResourceLoader_ObjectReference_Hard{*FoundLoadedAsset};

            return FCk_ResourceLoader_LoadedObject{LoadedObjectSoftRef, LoadedObjectHardRef}.Set_StreamableHandle(StreamingHandle);
        });

        const auto UniqueLoadedObjects = algo::Transform<TArray<FCk_ResourceLoader_LoadedObject>>(LoadedAssets,
        [&](UObject* InLoadedAsset)
        {
            const auto LoadedObjectSoftRef = FCk_ResourceLoader_ObjectReference_Soft{FSoftObjectPath(InLoadedAsset)};
            const auto LoadedObjectHardRef = FCk_ResourceLoader_ObjectReference_Hard{InLoadedAsset};

            return FCk_ResourceLoader_LoadedObject{LoadedObjectSoftRef, LoadedObjectHardRef}.Set_StreamableHandle(StreamingHandle);
        });

        const auto LoadedObjectBatch = FCk_ResourceLoader_LoadedObjectBatch{UniqueLoadedObjects, LoadedObjects}.Set_StreamableHandle(StreamingHandle);
        DoOnObjectBatchLoaded(InHandle, LoadedObjectBatch);
    }

    auto
        FProcessor_ResourceLoader_HandleRequests::
        DoOnObjectLoaded(
            HandleType InHandle,
            FCk_ResourceLoader_LoadedObject InObjectLoaded) const
        -> void
    {
        const auto Payload = FCk_Payload_ResourceLoader_OnObjectLoaded{InObjectLoaded};

        UUtils_Signal_ResourceLoader_OnObjectLoaded::Broadcast(InHandle, MakePayload(InHandle, Payload));

        const auto& ResourceLoaderSubsystem = UCk_Utils_ResourceLoader_Subsystem_UE::Get_Subsystem();

        CK_ENSURE_IF_NOT(ck::IsValid(ResourceLoaderSubsystem),
            TEXT("Could not retrieve Resource Loader subsystem! Unable to Track Loaded Object [{}]"),
            InObjectLoaded)
        { return; }

        ResourceLoaderSubsystem->Request_TrackResource(InObjectLoaded);
    }

    auto
        FProcessor_ResourceLoader_HandleRequests::
        DoOnObjectBatchLoaded(
            HandleType InHandle,
            FCk_ResourceLoader_LoadedObjectBatch InObjectBatchLoaded) const
        -> void
    {
        const auto Payload = FCk_Payload_ResourceLoader_OnObjectBatchLoaded{InObjectBatchLoaded};

        UUtils_Signal_ResourceLoader_OnObjectBatchLoaded::Broadcast(InHandle, MakePayload(InHandle, Payload));

        const auto& ResourceLoaderSubsystem = UCk_Utils_ResourceLoader_Subsystem_UE::Get_Subsystem();

        CK_ENSURE_IF_NOT(ck::IsValid(ResourceLoaderSubsystem),
            TEXT("Could not retrieve Resource Loader subsystem! Unable to Track Loaded Object Batch"))
        { return; }

        for (const auto& LoadedObject : InObjectBatchLoaded.Get_UniqueLoadedObjects())
        {
            ResourceLoaderSubsystem->Request_TrackResource(LoadedObject);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
