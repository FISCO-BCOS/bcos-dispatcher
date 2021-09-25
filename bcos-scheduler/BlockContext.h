#pragma once

#include "ExecutorManager.h"
#include "bcos-framework/interfaces/executor/ExecutionMessage.h"
#include "bcos-framework/interfaces/protocol/Block.h"
#include "bcos-framework/interfaces/protocol/BlockHeader.h"
#include "bcos-framework/interfaces/protocol/ProtocolTypeDef.h"
#include "bcos-framework/libutilities/Error.h"
#include "bcos-scheduler/ExecutorManager.h"
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
class BlockContext
{
public:
    using Ptr = std::shared_ptr<BlockContext>;
    using ConstPtr = std::shared_ptr<const BlockContext>;

    enum Status : int8_t
    {
        IDLE,
        EXECUTING,
        FINISHED,
    };

    BlockContext(bcos::protocol::Block::ConstPtr block) : m_block(std::move(block)) {}

    void asyncExecute(std::function<void(Error::UniquePtr&&)> callback) noexcept;

    bcos::protocol::BlockNumber number() { return m_block->blockHeaderConst()->number(); }

    Status status() { return m_status; }
    void setStatus(Status status) { m_status = status; }

private:
    void runBatch(std::function<void(Error::UniquePtr&&)> callback);

    struct ExecutiveState
    {
        ExecutiveState(int64_t _contextID) : contextID(_contextID) {}

        int64_t contextID;
        std::stack<int64_t> callStack;
        std::list<int64_t> callHistory;
        bcos::protocol::ExecutionMessage::UniquePtr message;
        int64_t m_currentSeq = 0;
    };

    struct KeyLock
    {
        int64_t contextID;
        std::atomic_int64_t count;
    };

    bcos::protocol::Block::ConstPtr m_block;

    std::list<ExecutiveState> m_executiveStates;
    std::vector<bcos::protocol::TransactionReceipt::Ptr> m_receipts;

    std::set<std::string, std::less<>> m_calledContract;
    tbb::concurrent_unordered_map<std::string, KeyLock> m_keyLocks;

    Status m_status = IDLE;
    ExecutorManager::Ptr m_executorManager;
    bcos::protocol::ExecutionMessageFactory::Ptr m_executionMessageFactory;
    bcos::protocol::TransactionReceiptFactory::Ptr m_transactionReceiptFactory;

    int64_t m_seqCount = 0;
};
}  // namespace bcos::scheduler