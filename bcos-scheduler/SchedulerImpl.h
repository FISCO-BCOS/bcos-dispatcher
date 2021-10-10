#pragma once

#include "BlockExecutive.h"
#include "ExecutorManager.h"
#include "bcos-framework/interfaces/dispatcher/SchedulerInterface.h"
#include "bcos-framework/interfaces/ledger/LedgerInterface.h"
#include <bcos-framework/interfaces/executor/ParallelTransactionExecutorInterface.h>
#include <tbb/concurrent_queue.h>
#include <list>

namespace bcos::scheduler
{
class SchedulerImpl : public SchedulerInterface
{
public:
    // by pbft & sync
    void executeBlock(const bcos::protocol::Block::ConstPtr& block, bool verify,
        std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)>
            callback) noexcept override;

    // by pbft & sync
    void commitBlock(const bcos::protocol::BlockHeader::ConstPtr& header,
        std::function<void(bcos::Error::Ptr&&, bcos::ledger::LedgerConfig::Ptr&&)>
            callback) noexcept override;

    // by console, query committed committing executing
    void status(std::function<void(Error::Ptr&&, bcos::protocol::Session::ConstPtr&&)>
            callback) noexcept override;

    // by rpc
    void call(const protocol::Transaction::ConstPtr& tx,
        std::function<void(Error::Ptr&&, protocol::TransactionReceipt::Ptr&&)>) noexcept override;

    // by executor
    void registerExecutor(const std::string& name,
        const bcos::executor::ParallelTransactionExecutorInterface::Ptr& executor,
        std::function<void(Error::Ptr&&)> callback) noexcept override;

    void unregisterExecutor(
        const std::string& name, std::function<void(Error::Ptr&&)> callback) noexcept override;

    void reset(std::function<void(Error::Ptr&&)> callback) noexcept override;

private:
    void execute();

    struct QueueItem
    {
        BlockExecutive::UniquePtr executive;
        bcos::protocol::BlockHeader::Ptr result;
        std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)> callback;
    };

    std::list<QueueItem> m_blocks;
    std::mutex m_blocksMutex;

    decltype(m_blocks)::iterator m_executing;
    std::mutex m_executeMutex;

    decltype(m_blocks)::iterator m_committing;
    std::mutex m_commitMutex;

    ExecutorManager::Ptr m_executorManager;
    bcos::ledger::LedgerInterface::Ptr m_ledger;
    bcos::storage::TransactionalStorageInterface::Ptr m_storage;
    bcos::protocol::ExecutionMessageFactory::Ptr m_executionMessageFactory;
    bcos::protocol::TransactionReceiptFactory::Ptr m_transactionReceiptFactory;
    bcos::protocol::BlockHeaderFactory::Ptr m_blockHeaderFactory;
    bcos::crypto::Hash::Ptr m_hashImpl;
};
}  // namespace bcos::scheduler