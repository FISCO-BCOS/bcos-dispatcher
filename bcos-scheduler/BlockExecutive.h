#pragma once

#include "ExecutorManager.h"
#include "bcos-framework/interfaces/executor/ExecutionMessage.h"
#include "bcos-framework/interfaces/protocol/Block.h"
#include "bcos-framework/interfaces/protocol/BlockHeader.h"
#include "bcos-framework/interfaces/protocol/ProtocolTypeDef.h"
#include "bcos-framework/libutilities/Error.h"
#include "bcos-scheduler/ExecutorManager.h"
#include "interfaces/protocol/BlockHeaderFactory.h"
#include "interfaces/protocol/TransactionMetaData.h"
#include "interfaces/protocol/TransactionReceiptFactory.h"
#include <tbb/concurrent_unordered_map.h>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/range/any_range.hpp>
#include <forward_list>
#include <mutex>
#include <stack>
#include <thread>

namespace bcos::scheduler
{
class BlockExecutive
{
public:
    using UniquePtr = std::unique_ptr<BlockExecutive>;

    enum Status : int8_t
    {
        IDLE,
        EXECUTING,
        FINISHED,
    };

    BlockExecutive(bcos::protocol::Block::ConstPtr block, ExecutorManager::Ptr executorManager,
        bcos::protocol::ExecutionMessageFactory::Ptr executionMessageFactory,
        bcos::protocol::TransactionReceiptFactory::Ptr transactionReceiptFactory,
        bcos::protocol::BlockHeaderFactory::Ptr blockHeaderFactory)
      : m_block(std::move(block)),
        m_executorManager(std::move(executorManager)),
        m_executionMessageFactory(std::move(executionMessageFactory)),
        m_transactionReceiptFactory(std::move(transactionReceiptFactory)),
        m_blockHeaderFactory(std::move(blockHeaderFactory))
    {}

    void asyncExecute(
        std::function<void(Error::UniquePtr&&, protocol::BlockHeader::Ptr)> callback) noexcept;

    bcos::protocol::BlockNumber number() { return m_block->blockHeaderConst()->number(); }

    Status status() { return m_status; }
    void setStatus(Status status) { m_status = status; }

private:
    struct BatchStatus  // Batch state per batch
    {
        std::atomic_size_t total = 0;
        std::atomic_size_t received = 0;

        std::function<void(Error::UniquePtr&&)> callback;
        std::atomic_bool callbackExecuted = false;
        std::atomic_bool allSended = false;
    };
    void startBatch(std::function<void(Error::UniquePtr&&)> callback);
    void checkBatch(BatchStatus& status);
    protocol::BlockHeader::Ptr generateResultBlockHeader();

    struct ExecutiveState  // Executive state per tx
    {
        ExecutiveState(int64_t _contextID) : contextID(_contextID) {}

        int64_t contextID;
        std::stack<int64_t, std::list<int64_t>> callStack;
        std::list<int64_t> callHistory;
        bcos::protocol::ExecutionMessage::UniquePtr message;
        bcos::Error::UniquePtr error;
        int64_t currentSeq = 0;
        std::set<std::tuple<std::string, std::string>> keyLocks;
    };
    struct ExecutiveResult
    {
        bcos::protocol::TransactionReceipt::Ptr receipt;
    };
    std::list<ExecutiveState> m_executiveStates;
    std::vector<ExecutiveResult> m_executiveResults;

    std::set<std::string, std::less<>> m_calledContract;

    struct KeyLock
    {
        int64_t contextID;
        std::atomic_int64_t count;
    };
    tbb::concurrent_unordered_map<std::string, KeyLock> m_keyLocks;

    Status m_status = IDLE;
    bcos::protocol::Block::ConstPtr m_block;
    ExecutorManager::Ptr m_executorManager;
    bcos::protocol::ExecutionMessageFactory::Ptr m_executionMessageFactory;
    bcos::protocol::TransactionReceiptFactory::Ptr m_transactionReceiptFactory;
    bcos::protocol::BlockHeaderFactory::Ptr m_blockHeaderFactory;

    int64_t m_seqCount = 0;
};
}  // namespace bcos::scheduler