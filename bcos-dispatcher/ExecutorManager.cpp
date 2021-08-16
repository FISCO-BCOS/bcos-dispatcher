#include "ExecutorManager.h"
#include "libutilities/Common.h"
#include "libutilities/Exceptions.h"
#include <bcos-framework/libutilities/Exceptions.h>
#include <tbb/parallel_sort.h>
#include <algorithm>

using namespace bcos::dispatcher;

void ExecutorManager::addExecutor(
    const std::string& name, const bcos::executor::ParallelExecutorInterface::Ptr& executor)
{
    std::unique_lock lock(m_mutex);

    auto executorInfo = std::make_shared<ExecutorInfo>();
    executorInfo->executor = executor;

    auto [it, exists] = m_name2Executors.emplace(name, std::move(executorInfo));

    if (exists)
    {
        throw bcos::Exception("Executor already exists");
    }

    std::unique_lock lockHeap(m_heapMutex);
    m_executorsHeap.push_back(executorInfo);
    std::push_heap(m_executorsHeap.begin(), m_executorsHeap.end(), m_executorComp);
}

void ExecutorManager::removeExecutor(const std::string& name)
{
    std::unique_lock lock(m_mutex);

    auto it = m_name2Executors.find(name);
    if (it != m_name2Executors.end())
    {
        auto& executorInfo = it->second;

        for (auto contractIt = executorInfo->contracts.begin();
             contractIt != executorInfo->contracts.end(); ++contractIt)
        {
            auto count = m_contract2ExecutorInfo.unsafe_erase(*contractIt);
            if (count < 1)
            {
                throw bcos::Exception("Can't find contract in container");
            }
        }

        m_name2Executors.unsafe_erase(it);
    }
}