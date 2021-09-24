#include "ExecutorManager.h"
#include <bcos-framework/libutilities/Error.h>
#include <tbb/parallel_sort.h>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <mutex>
#include <thread>

using namespace bcos::scheduler;

void ExecutorManager::addExecutor(
    std::string name, const bcos::executor::ParallelTransactionExecutorInterface::Ptr& executor)
{
    auto executorInfo = std::make_shared<ExecutorInfo>();
    executorInfo->name = std::move(name);
    executorInfo->executor = executor;

    std::unique_lock lock(m_mutex);
    auto [it, exists] = m_name2Executors.emplace(executorInfo->name, executorInfo);

    if (!exists)
    {
        BOOST_THROW_EXCEPTION(bcos::Exception("Executor already exists"));
    }

    m_executorQueue.push(executorInfo);
}

bcos::executor::ParallelTransactionExecutorInterface::Ptr ExecutorManager::dispatchExecutor(
    const std::string_view& contract)
{
    executor::ParallelTransactionExecutorInterface::Ptr executor;

    do
    {
        auto executorIt = m_contract2ExecutorInfo.find(contract);
        if (executorIt != m_contract2ExecutorInfo.end())
        {
            executor = executorIt->second->executor;
        }
        else
        {
            std::unique_lock lock(m_mutex, std::try_to_lock);
            if (!lock.owns_lock())
            {
                continue;
            }

            auto executorInfo = m_executorQueue.top();
            m_executorQueue.pop();

            auto [contractStr, success] = executorInfo->contracts.insert(std::string(contract));
            if (!success)
            {
                BOOST_THROW_EXCEPTION(BCOS_ERROR(-1, "Insert into contracts fail!"));
            }
            m_executorQueue.push(executorInfo);

            (void)m_contract2ExecutorInfo.emplace(*contractStr, executorInfo);

            executor = executorInfo->executor;
        }
    } while (false);

    return executor;
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

        m_name2Executors.erase(it);

        m_executorQueue = std::priority_queue<ExecutorInfo::Ptr, std::vector<ExecutorInfo::Ptr>,
            ExecutorInfoComp>();

        for (auto& it : m_name2Executors)
        {
            m_executorQueue.push(it.second);
        }
    }
    else
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR(-1, "Not found executor: " + std::string(name)));
    }
}