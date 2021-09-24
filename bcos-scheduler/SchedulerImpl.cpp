#include "SchedulerImpl.h"
#include "Common.h"
#include <boost/exception/diagnostic_information.hpp>

using namespace bcos::scheduler;

void SchedulerImpl::executeBlock(const bcos::protocol::Block::ConstPtr& block, bool verify,
    std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)> callback) noexcept
{
    if (!m_blockContexts.empty())
    {
        // Check if the block number is valid
        auto currentBlockContext = m_blockContexts.back();

        if (block->blockHeaderConst()->number() - currentBlockContext->number() != 1)
        {
            // wrong number
            callback(BCOS_ERROR_PTR(-1,
                         "Wrong request block number: " +
                             boost::lexical_cast<std::string>(block->blockHeaderConst()->number()) +
                             " current number: " +
                             boost::lexical_cast<std::string>(currentBlockContext->number())),
                nullptr);

            return;
        }
    }

    auto blockContext = std::make_shared<BlockContext>(block);

    (void)block;
    (void)verify;
    (void)callback;
}

// by pbft & sync
void SchedulerImpl::commitBlock(const bcos::protocol::BlockHeader::ConstPtr& header,
    std::function<void(bcos::Error::Ptr&&, bcos::ledger::LedgerConfig::Ptr&&)> callback) noexcept
{
    (void)header;
    (void)callback;
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