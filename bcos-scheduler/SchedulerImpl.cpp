#include "SchedulerImpl.h"
#include "Common.h"
#include "libutilities/Error.h"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <mutex>

using namespace bcos::scheduler;

void SchedulerImpl::executeBlock(bcos::protocol::Block::Ptr block, bool verify,
    std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)> callback) noexcept
{
    SCHEDULER_LOG(INFO) << "ExecuteBlock request"
                        << LOG_KV("block number", block->blockHeaderConst()->number())
                        << LOG_KV("verify", verify);

    {
        std::scoped_lock<std::mutex> lock(m_blocksMutex);
        if (!m_blocks.empty())
        {
            auto startNum = m_blocks.front().executive->number();
            auto executingNum = m_executing->executive->number();
            auto endNum = m_blocks.back().executive->number();

            auto currentNum = block->blockHeaderConst()->number();

            // Block already executed
            if (currentNum >= startNum && currentNum < executingNum)
            {
                SCHEDULER_LOG(INFO) << "ExecuteBlock success, return executed block"
                                    << LOG_KV("block number", block->blockHeaderConst()->number())
                                    << LOG_KV("verify", verify);

                auto it = m_blocks.begin();
                while (it->executive->number() != currentNum)
                {
                    ++it;
                }

                callback(nullptr, bcos::protocol::BlockHeader::Ptr(it->executive->result()));
                return;
            }

            // Block not yet execute
            if (currentNum - endNum != 1)
            {
                auto message =
                    "Invalid block number: " +
                    boost::lexical_cast<std::string>(block->blockHeaderConst()->number()) +
                    " current last number: " + boost::lexical_cast<std::string>(endNum);
                SCHEDULER_LOG(ERROR) << "ExecuteBlock error, " << message;

                callback(BCOS_ERROR_PTR(SchedulerError::InvalidBlockNumber, std::move(message)),
                    nullptr);

                return;
            }
        }

        m_blocks.emplace_back(BlockExecuteItem{
            std::make_unique<BlockExecutive>(block, m_executorManager, m_executionMessageFactory,
                m_transactionReceiptFactory, m_blockHeaderFactory, m_hashImpl),
            std::move(callback)});
    }

    execute();

    SCHEDULER_LOG(INFO) << "ExecuteBlock success"
                        << LOG_KV("block number", block->blockHeaderConst()->number());
}

// by pbft & sync
void SchedulerImpl::commitBlock(bcos::protocol::BlockHeader::Ptr header,
    std::function<void(bcos::Error::Ptr&&, bcos::ledger::LedgerConfig::Ptr&&)> callback) noexcept
{
    SCHEDULER_LOG(INFO) << "CommitBlock request" << LOG_KV("block number", header->number());

    {
        std::scoped_lock<std::mutex> lock(m_blocksMutex);

        if (!m_blocks.empty())
        {
            auto& frontBlock = m_blocks.front();
            if (!frontBlock.executive->result())
            {
                auto message = "Block executing";
                SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
                callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);
                return;
            }

            // m_ledger->asyncPrewriteBlock(frontBlock.executive->, bcos::protocol::Block::ConstPtr
            // block, std::function<void (Error::Ptr &&)> callback)

            // auto block =

            //     m_ledger->asyncPrewriteBlock(m_storage, bcos::protocol::Block::ConstPtr block,
            //         std::function<void(Error::Ptr &&)> callback)
        }
        else
        {
            auto message = "Empty m_blocks";
            SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
            callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlocks, message), nullptr);
            return;
        }
    }

    // TODO: Commit the block
}

// by console, query committed committing executing
void SchedulerImpl::status(
    std::function<void(Error::Ptr&&, bcos::protocol::Session::ConstPtr&&)> callback) noexcept
{
    (void)callback;
}

// by rpc
void SchedulerImpl::call(protocol::Transaction::Ptr tx,
    std::function<void(Error::Ptr&&, protocol::TransactionReceipt::Ptr&&)> callback) noexcept
{
    (void)tx;
    (void)callback;
}

void SchedulerImpl::registerExecutor(std::string name,
    bcos::executor::ParallelTransactionExecutorInterface::Ptr executor,
    std::function<void(Error::Ptr&&)> callback) noexcept
{
    try
    {
        SCHEDULER_LOG(INFO) << "registerExecutor request: " << LOG_KV("name", name);
        m_executorManager->addExecutor(name, executor);
    }
    catch (std::exception& e)
    {
        SCHEDULER_LOG(ERROR) << "registerExecutor error: " << boost::diagnostic_information(e);
        callback(BCOS_ERROR_WITH_PREV_PTR(-1, "addExecutor error", e));
        return;
    }

    SCHEDULER_LOG(INFO) << "registerExecutor success";
    callback(nullptr);
}

void SchedulerImpl::unregisterExecutor(
    const std::string& name, std::function<void(Error::Ptr&&)> callback) noexcept
{
    (void)name;
    (void)callback;
}

void SchedulerImpl::reset(std::function<void(Error::Ptr&&)> callback) noexcept
{
    (void)callback;
}

void SchedulerImpl::execute()
{
    auto executeLock = std::unique_lock<std::mutex>(m_executeMutex, std::try_to_lock);
    if (!executeLock.owns_lock())
    {
        // Another rountine is executting block
        return;
    }

    if (m_executing == m_blocks.end())
    {
        // All block is finished
        return;
    }

    auto executeLockPtr = std::make_shared<std::unique_lock<std::mutex>>(std::move(executeLock));
    m_executing->executive->asyncExecute(
        [this, executeLock = std::move(executeLockPtr)](
            Error::UniquePtr&& error, protocol::BlockHeader::Ptr blockHeader) mutable {
            auto it = m_executing;
            ++m_executing;

            executeLock->release();

            it->callback(std::move(error), bcos::protocol::BlockHeader::Ptr(blockHeader));

            execute();
        });
}