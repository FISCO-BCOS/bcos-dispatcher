#pragma once

#include <bcos-framework/interfaces/executor/ParallelTransactionExecutorInterface.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/parallel_for.h>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/range/any_range.hpp>
#include <iterator>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace bcos::dispatcher
{
class ExecutorManager
{
public:
    using Ptr = std::shared_ptr<ExecutorManager>;

    void addExecutor(std::string name,
        const bcos::executor::ParallelTransactionExecutorInterface::Ptr& executor);

    std::vector<bcos::executor::ParallelTransactionExecutorInterface::Ptr> dispatchExecutor(
        boost::any_range<std::string_view, boost::random_access_traversal_tag> contracts);

    void removeExecutor(const std::string_view& name);

private:
    struct ExecutorInfo
    {
        using Ptr = std::shared_ptr<ExecutorInfo>;

        std::string name;
        bcos::executor::ParallelTransactionExecutorInterface::Ptr executor;
        std::set<std::string> contracts;
    };

    struct ExecutorInfoComp
    {
        bool operator()(const ExecutorInfo::Ptr& lhs, const ExecutorInfo::Ptr& rhs) const
        {
            return lhs->contracts.size() > rhs->contracts.size();
        }
    } m_executorComp;

    tbb::concurrent_unordered_map<std::string_view, ExecutorInfo::Ptr, std::hash<std::string_view>>
        m_contract2ExecutorInfo;
    tbb::concurrent_unordered_map<std::string_view, ExecutorInfo::Ptr, std::hash<std::string_view>>
        m_name2Executors;
    std::shared_mutex m_mutex;

    std::vector<ExecutorInfo::Ptr> m_executorsHeap;
    std::shared_mutex m_heapMutex;
};
}  // namespace bcos::dispatcher