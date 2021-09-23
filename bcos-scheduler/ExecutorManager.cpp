#include "ExecutorManager.h"
#include <bcos-framework/libutilities/Error.h>
#include <tbb/parallel_sort.h>
#include <boost/throw_exception.hpp>
#include <algorithm>

using namespace bcos::dispatcher;

void ExecutorManager::addExecutor(
    std::string name, const bcos::executor::ParallelTransactionExecutorInterface::Ptr& executor)
{
    std::unique_lock lock(m_mutex);

    auto executorInfo = std::make_shared<ExecutorInfo>();
    executorInfo->name = std::move(name);
    executorInfo->executor = executor;

    auto [it, exists] = m_name2Executors.emplace(executorInfo->name, executorInfo);

    if (!exists)
    {
        BOOST_THROW_EXCEPTION(bcos::Exception("Executor already exists"));
    }

    m_executorsHeap.push_back(executorInfo);
    std::push_heap(m_executorsHeap.begin(), m_executorsHeap.end(), m_executorComp);
}

std::vector<bcos::executor::ParallelTransactionExecutorInterface::Ptr>
ExecutorManager::dispatchExecutor(
    boost::any_range<std::string_view, boost::random_access_traversal_tag> contracts)
{
    std::unique_lock lock(m_mutex);

    std::vector<bcos::executor::ParallelTransactionExecutorInterface::Ptr> result;
    result.reserve(contracts.size());

    for (auto& it : contracts)
    {
        auto executorIt = m_contract2ExecutorInfo.find(it);

        executor::ParallelTransactionExecutorInterface::Ptr executor;
        if (executorIt != m_contract2ExecutorInfo.end())
        {
            executor = executorIt->second->executor;
        }
        else
        {
            std::pop_heap(m_executorsHeap.begin(), m_executorsHeap.end(), m_executorComp);
            auto executorInfo = m_executorsHeap.back();

            auto [contractIt, success] = executorInfo->contracts.insert(std::string(it));
            (void)success;
            std::push_heap(m_executorsHeap.begin(), m_executorsHeap.end(), m_executorComp);

            (void)m_contract2ExecutorInfo.insert({*contractIt, executorInfo});

            executor = executorInfo->executor;
        }

        result.push_back(executor);
    }

    return result;
}

void ExecutorManager::removeExecutor(const std::string_view& name)
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
                BOOST_THROW_EXCEPTION(bcos::Exception("Can't find contract in container"));
            }
        }

        m_name2Executors.unsafe_erase(it);
    }
    else
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR(-1, "Not found executor: " + std::string(name)));
    }

    bool deleted = false;
    for (auto heapIt = m_executorsHeap.begin(); heapIt != m_executorsHeap.end(); ++heapIt)
    {
        std::cout << "origin: " << (*heapIt)->name << " target: " << name << std::endl;
        if ((*heapIt)->name == name)
        {
            m_executorsHeap.erase(heapIt);
            deleted = true;
            break;
        }
    }

    if (!deleted)
    {
        BOOST_THROW_EXCEPTION(bcos::Exception("Can't find executor in heap"));
    }
}