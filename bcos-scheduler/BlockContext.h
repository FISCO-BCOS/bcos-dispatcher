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
#include <stack>

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

    struct BatchState
    {
        std::stack<int64_t> callStack;
        std::forward_list<int64_t> callHistory;
        bcos::protocol::ExecutionMessage::UniquePtr message;
        bcos::protocol::TransactionReceipt::Ptr receipt;
    };

    bcos::protocol::Block::ConstPtr m_block;
    std::vector<BatchState> m_batchStates;

    std::unordered_set<std::string_view> m_calledContract;
    tbb::concurrent_unordered_set<std::string_view> m_keyLocks;

    Status m_status = IDLE;
    ExecutorManager::Ptr m_executorManager;
    bcos::protocol::ExecutionMessageFactory::Ptr m_executionMessageFactory;
    bcos::protocol::TransactionReceiptFactory::Ptr m_transactionReceiptFactory;

    int64_t m_seqCount = 0;
};
}  // namespace bcos::scheduler