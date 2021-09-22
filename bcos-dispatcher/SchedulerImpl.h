#pragma once

#include "ExecutorManager.h"
#include "bcos-framework/interfaces/dispatcher/SchedulerInterface.h"
#include "bcos-framework/interfaces/executor/ParallelExecutorInterface.h"
#include "bcos-framework/interfaces/ledger/LedgerInterface.h"
#include <list>

namespace bcos
{
namespace dispatcher
{
class SchedulerImpl : public SchedulerInterface
{
public:
    // by pbft & sync
    void executeBlock(const bcos::protocol::Block::ConstPtr& block, bool verify,
        std::function<void(const bcos::Error::ConstPtr&, bcos::protocol::BlockHeader::Ptr&&)>
            callback) noexcept override;

    // by pbft & sync
    void commitBlock(const bcos::protocol::BlockHeader::ConstPtr& header,
        std::function<void(const bcos::Error::ConstPtr&)>) noexcept override;

    // by console, query committed committing executing
    void status(
        std::function<void(const Error::ConstPtr&, const bcos::protocol::Session::ConstPtr&)>
            callback) noexcept override;

    // by rpc
    void callTransaction(const protocol::Transaction::ConstPtr& tx,
        std::function<void(const Error::ConstPtr&, protocol::TransactionReceipt::Ptr&&)>) noexcept
        override;

    // by executor
    void registerExecutor(std::function<void(const Error::ConstPtr&)> callback,
        std::string&& executorID) noexcept override;

    // by executor
    void notifyCommitResult(const std::string_view& executorID,
        bcos::protocol::BlockNumber blockNumber,
        std::function<void(const Error::ConstPtr&)> callback) noexcept override;

    void reset(std::function<void(const Error::ConstPtr&)> callback) noexcept override;

private:
    ExecutorManager::Ptr m_executorManager;

    bcos::ledger::LedgerInterface::Ptr m_ledger;
    void* m_storage;  // TODO: replace to storage
};
}  // namespace dispatcher
}  // namespace bcos