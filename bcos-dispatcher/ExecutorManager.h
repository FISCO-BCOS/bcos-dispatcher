#pragma once

#include "bcos-framework/interfaces/executor/ParallelExecutorInterface.h"
#include <tbb/blocked_range.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/parallel_for.h>
#include <iterator>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace bcos
{
namespace dispatcher
{
class ExecutorManager
{
public:
    using Ptr = std::shared_ptr<ExecutorManager>;

    void addExecutor(
        const std::string& name, const bcos::executor::ParallelExecutorInterface::Ptr& executor);

    template <class Iterator>
    std::vector<bcos::executor::ParallelExecutorInterface::Ptr> dispatchExecutor(
        Iterator begin, Iterator end)
    {
        std::unique_lock lock(m_mutex);

        typename Iterator::difference_type size = end - begin;
        std::vector<bcos::executor::ParallelExecutorInterface::Ptr> result(size);

        tbb::parallel_for(tbb::blocked_range<typename Iterator::difference_type>(0, size),
            [this, &result, &begin](
                const tbb::blocked_range<typename Iterator::difference_type>& range) {
                for (auto it = range.begin(); it != range.end(); ++it)
                {
                    auto& contract = *(begin + it);
                    auto executorIt = m_contract2ExecutorInfo.find(contract);
                    if (executorIt != m_contract2ExecutorInfo.end())
                    {
                        result[it] = executorIt->second->executor;
                    }
                    else
                    {
                        std::unique_lock heapLock(m_heapMutex);

                        std::pop_heap(
                            m_executorsHeap.begin(), m_executorsHeap.end(), m_executorComp);
                        auto executorInfo = m_executorsHeap.back();
                        executorInfo->contracts.insert(*(begin + it));
                        std::push_heap(
                            m_executorsHeap.begin(), m_executorsHeap.end(), m_executorComp);

                        m_contract2ExecutorInfo.emplace(contract, executorInfo);

                        result[it] = executorInfo->executor;
                    }
                }
            });

        return result;
    }

    void removeExecutor(const std::string& name);

private:
    struct ExecutorInfo
    {
        using Ptr = std::shared_ptr<ExecutorInfo>;

        bcos::executor::ParallelExecutorInterface::Ptr executor;
        std::set<std::string> contracts;
    };

    struct ExecutorInfoComp
    {
        bool operator()(const ExecutorInfo::Ptr& lhs, const ExecutorInfo::Ptr& rhs) const
        {
            return lhs->contracts.size() > rhs->contracts.size();
        }
    } m_executorComp;

    tbb::concurrent_unordered_map<std::string, ExecutorInfo::Ptr> m_contract2ExecutorInfo;
    tbb::concurrent_unordered_map<std::string, ExecutorInfo::Ptr> m_name2Executors;
    std::shared_mutex m_mutex;

    std::vector<ExecutorInfo::Ptr> m_executorsHeap;
    std::shared_mutex m_heapMutex;
};
}  // namespace dispatcher
}  // namespace bcos