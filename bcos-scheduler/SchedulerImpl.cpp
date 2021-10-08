#include "SchedulerImpl.h"
#include "Common.h"
#include "libutilities/Error.h"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <mutex>

using namespace bcos::scheduler;

void SchedulerImpl::executeBlock(const bcos::protocol::Block::ConstPtr& block, bool verify,
    std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)> callback) noexcept
{
    SCHEDULER_LOG(INFO) << "ExecuteBlock request"
                        << LOG_KV("block number", block->blockHeaderConst()->number())
                        << LOG_KV("verify", verify);

    {
        // TODO: 做哈希的缓存，防止重复执行
        // 重复的区块，没结果时报错，有结果时返回结果
        // 结论：（同步、PBFT）串行下发区块
        // 用区块哈希去重

        std::scoped_lock<std::mutex> lock(m_blockContextsMutex);
        if (!m_blocks.empty())
        {
            // Check if the block number is valid
            auto& currentBlockContext = std::get<0>(m_blocks.back());

            // TODO: 已执行 > 当前块高 > 已提交，返回已执行的结果
            if (block->blockHeaderConst()->number() - currentBlockContext->number() != 1)
            {
                auto message =
                    "Invalid block number: " +
                    boost::lexical_cast<std::string>(block->blockHeaderConst()->number()) +
                    " current number: " +
                    boost::lexical_cast<std::string>(currentBlockContext->number());
                SCHEDULER_LOG(ERROR) << "ExecuteBlock error: " << message;

                callback(BCOS_ERROR_PTR(SchedulerError::InvalidBlockNumber, std::move(message)),
                    nullptr);

                return;
            }
        }

        auto blockContext = std::make_unique<BlockExecutive>(block, m_executorManager,
            m_executionMessageFactory, m_transactionReceiptFactory, m_blockHeaderFactory);

        m_blocks.emplace_back(std::move(blockContext), std::move(callback));
    }

    execute();

    SCHEDULER_LOG(INFO) << "ExecuteBlock success"
                        << LOG_KV("block number", block->blockHeaderConst()->number());
}

// by pbft & sync
void SchedulerImpl::commitBlock(const bcos::protocol::BlockHeader::ConstPtr& header,
    std::function<void(bcos::Error::Ptr&&, bcos::ledger::LedgerConfig::Ptr&&)> callback) noexcept
{
    SCHEDULER_LOG(INFO) << "CommitBlock request" << LOG_KV("block number", header->number());

    {
        std::scoped_lock<std::mutex> lock(m_blockContextsMutex);

        if (m_blocks.empty())
        {
            auto message = "Empty m_blocks";

            SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
            callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlocks, message), nullptr);

            return;
        }

        if (m_blockCommitting != m_blocks.end())
        {
            auto message = "Another block is committing";

            SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
            callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);

            return;
        }

        auto it = m_blocks.begin();
        if (std::get<0>(*it)->number() != header->number())
        {
            auto message = "Block mismatch, last executed number: " +
                           boost::lexical_cast<std::string>(std::get<0>(*it)->number()) +
                           " request number: " + boost::lexical_cast<std::string>(header->number());

            SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
            callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlockNumber, message), nullptr);

            return;
        }

        m_blockCommitting = it;
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
void SchedulerImpl::call(const protocol::Transaction::ConstPtr& tx,
    std::function<void(Error::Ptr&&, protocol::TransactionReceipt::Ptr&&)> callback) noexcept
{
    (void)tx;
    (void)callback;
}

void SchedulerImpl::registerExecutor(const std::string& name,
    const bcos::executor::ParallelTransactionExecutorInterface::Ptr& executor,
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
    std::scoped_lock<std::mutex> lock(m_blockContextsMutex);
    if (m_blockExecuting != m_blocks.end())
    {
        // Another execute is running
        return;
    }

    if (m_blocks.empty())
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR(SchedulerError::InvalidBlocks, "No block in m_blocks"));
    }

    m_blockExecuting = m_blocks.begin();

    std::get<0>(*m_blockExecuting)
        ->asyncExecute([this](Error::UniquePtr&& error, protocol::BlockHeader::Ptr blockHeader) {
            std::scoped_lock<std::mutex> lock(m_blockContextsMutex);  // TODO: need recursive_mutex

            ++m_blockExecuting;

            std::get<1> (*m_blockExecuting)(std::move(error), std::move(blockHeader));
        });
}