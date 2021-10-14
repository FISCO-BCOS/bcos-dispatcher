#include "SchedulerImpl.h"
#include "Common.h"
#include "interfaces/ledger/LedgerConfig.h"
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

    std::unique_lock<std::mutex> executeLock(m_executeMutex, std::try_to_lock);
    if (!executeLock.owns_lock())
    {
        auto message = "Another block is executing!";
        SCHEDULER_LOG(ERROR) << "ExecuteBlock error, " << message;
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);
        return;
    }

    std::unique_lock<std::mutex> blocksLock(m_blocksMutex);

    if (!m_blocks.empty())
    {
        auto requestNumber = block->blockHeaderConst()->number();
        auto& frontBlock = m_blocks.front();
        auto& backBlock = m_blocks.back();

        // Block already executed
        if (requestNumber >= frontBlock.number() && requestNumber < backBlock.number())
        {
            SCHEDULER_LOG(INFO) << "ExecuteBlock success, return executed block"
                                << LOG_KV("block number", block->blockHeaderConst()->number())
                                << LOG_KV("verify", verify);

            auto it = m_blocks.begin();
            while (it->number() != requestNumber)
            {
                ++it;
            }

            callback(nullptr, bcos::protocol::BlockHeader::Ptr(it->result()));
            return;
        }

        if (requestNumber - backBlock.number() != 1)
        {
            auto message =
                "Invalid block number: " +
                boost::lexical_cast<std::string>(block->blockHeaderConst()->number()) +
                " current last number: " + boost::lexical_cast<std::string>(backBlock.number());
            SCHEDULER_LOG(ERROR) << "ExecuteBlock error, " << message;

            callback(
                BCOS_ERROR_PTR(SchedulerError::InvalidBlockNumber, std::move(message)), nullptr);

            return;
        }
    }

    m_blocks.emplace_back(std::move(block), this);

    auto executeLockPtr = std::make_shared<decltype(executeLock)>(std::move(executeLock));
    m_blocks.back().asyncExecute([callback = std::move(callback), executeLock =
                                                                      std::move(executeLockPtr)](
                                     Error::UniquePtr&& error, protocol::BlockHeader::Ptr header) {
        if (error)
        {
            SCHEDULER_LOG(ERROR) << "Unknown error, " << boost::diagnostic_information(*error);
            callback(
                BCOS_ERROR_WITH_PREV_PTR(SchedulerError::UnknownError, "Unknown error", *error),
                nullptr);
            return;
        }
        SCHEDULER_LOG(INFO) << "ExecuteBlock success" << LOG_KV("block number", header->number());
        callback(std::move(error), std::move(header));
    });
}

// by pbft & sync
void SchedulerImpl::commitBlock(bcos::protocol::BlockHeader::Ptr header,
    std::function<void(bcos::Error::Ptr&&, bcos::ledger::LedgerConfig::Ptr&&)> callback) noexcept
{
    SCHEDULER_LOG(INFO) << "CommitBlock request" << LOG_KV("block number", header->number());

    std::unique_lock<std::mutex> commitLock(m_commitMutex, std::try_to_lock);
    if (!commitLock.owns_lock())
    {
        auto message = "Another block is commiting!";
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);
        return;
    }

    std::unique_lock<std::mutex> blocksLock(m_blocksMutex);

    if (m_blocks.empty())
    {
        auto message = "No uncommitted block";
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlocks, message), nullptr);
        return;
    }

    auto& frontBlock = m_blocks.front();
    if (!frontBlock.result())
    {
        auto message = "Block is executing";
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);
        return;
    }

    if (header->number() != frontBlock.number())
    {
        auto message = "Invalid block number, available block number: " +
                       boost::lexical_cast<std::string>(frontBlock.number());
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlockNumber, message), nullptr);
        return;
    }

    auto commitLockPtr = std::make_shared<decltype(commitLock)>(
        std::move(commitLock));  // std::function need copyable

    blocksLock.unlock();
    frontBlock.asyncCommit([this, callback = std::move(callback),
                               commitLock = std::move(commitLockPtr)](Error::UniquePtr&& error) {
        if (error)
        {
            SCHEDULER_LOG(ERROR) << "CommitBlock error, " << boost::diagnostic_information(*error);
            callback(BCOS_ERROR_WITH_PREV_UNIQUE_PTR(
                         SchedulerError::UnknownError, "Unknown error", *error),
                nullptr);
            return;
        }

        std::unique_lock<std::mutex> blocksLock(m_blocksMutex);
        m_blocks.pop_front();

        std::string_view configs[] = {ledger::SYSTEM_KEY_TX_COUNT_LIMIT,
            ledger::SYSTEM_KEY_CONSENSUS_TIMEOUT, ledger::SYSTEM_KEY_CONSENSUS_LEADER_PERIOD};
        m_storage->asyncGetRows(ledger::SYS_CONFIG, configs,
            [callback = std::move(callback)](
                Error::UniquePtr&& error, std::vector<std::optional<storage::Entry>>&& entries) {
                if (error)
                {
                    SCHEDULER_LOG(ERROR)
                        << "Get system config error, " << boost::diagnostic_information(*error);
                    callback(BCOS_ERROR_WITH_PREV_UNIQUE_PTR(
                                 SchedulerError::UnknownError, "Get system config error", *error),
                        nullptr);
                    return;
                }

                if (entries.size() < 3)
                {
                    SCHEDULER_LOG(ERROR) << "Too few entries: " << entries.size();
                    callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::UnknownError, "Too few entries"),
                        nullptr);
                    return;
                }

                try
                {
                    auto ledgerConfig = std::make_shared<ledger::LedgerConfig>();
                    ledgerConfig->setBlockTxCountLimit(
                        boost::lexical_cast<uint64_t>(entries[0].value().getField(0)));
                    ledgerConfig->setConsensusTimeout(
                        boost::lexical_cast<uint64_t>(entries[1].value().getField(0)));
                    ledgerConfig->setLeaderSwitchPeriod(
                        boost::lexical_cast<uint64_t>(entries[2].value().getField(0)));

                    callback(nullptr, std::move(ledgerConfig));
                }
                catch (std::exception& e)
                {
                    SCHEDULER_LOG(ERROR)
                        << "Parse ledger config error, " << boost::diagnostic_information(e);
                    callback(BCOS_ERROR_WITH_PREV_UNIQUE_PTR(
                                 SchedulerError::UnknownError, "Parse ledger config error", e),
                        nullptr);
                    return;
                }
            });
    });
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