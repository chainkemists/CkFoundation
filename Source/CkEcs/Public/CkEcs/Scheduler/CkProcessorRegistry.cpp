#include "CkProcessorRegistry.h"

#include "CkEcs/CkEcsLog.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorRegistry::
    Get()
    -> FProcessorRegistry&
{
    static FProcessorRegistry Instance;
    return Instance;
}

auto
    ck::FProcessorRegistry::
    Register(
        FProcessorDescriptor&& InDescriptor)
    -> void
{
    FScopeLock Lock{&_Mutex};

    const auto ExistingIndex = _Descriptors.IndexOfByPredicate(
        [&](const FProcessorDescriptor& InExisting)
        {
            return InExisting._Name == InDescriptor._Name;
        });

    if (ExistingIndex != INDEX_NONE)
    {
        ck::ecs::Warning(TEXT("Processor [{}] already registered (hot-reload?). Replacing."),
            InDescriptor._Name);
        _Descriptors[ExistingIndex] = MoveTemp(InDescriptor);
        return;
    }

    ck::ecs::VeryVerbose(TEXT("Registering processor [{}]"), InDescriptor._Name);
    _Descriptors.Emplace(MoveTemp(InDescriptor));
}

auto
    ck::FProcessorRegistry::
    Deregister(
        FName InProcessorName)
    -> void
{
    FScopeLock Lock{&_Mutex};

    const auto RemovedCount = _Descriptors.RemoveAll(
        [&](const FProcessorDescriptor& InDescriptor)
        {
            return InDescriptor._Name == InProcessorName;
        });

    if (RemovedCount > 0)
    {
        ck::ecs::VeryVerbose(TEXT("Deregistered processor [{}]"), InProcessorName);
    }
}

auto
    ck::FProcessorRegistry::
    DeregisterAllFromModule(
        FName InModuleName)
    -> void
{
    // TODO Phase 5: Requires module name tracking in FProcessorDescriptor.
    //   Individual FAutoProcessorRegistrar destructors handle per-processor deregistration.
    //   This bulk method is reserved for FModuleManager::OnModulesChanged() integration.
    ck::ecs::Warning(TEXT("DeregisterAllFromModule([{}]) not yet implemented — individual registrar destructors handle cleanup."),
        InModuleName);
}

auto
    ck::FProcessorRegistry::
    Get_AllDescriptors() const
    -> const TArray<FProcessorDescriptor>&
{
    return _Descriptors;
}

// --------------------------------------------------------------------------------------------------------------------
