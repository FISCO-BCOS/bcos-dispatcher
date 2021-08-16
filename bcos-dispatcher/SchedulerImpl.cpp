#include "SchedulerImpl.h"

using namespace bcos::dispatcher;

void SchedulerImpl::executeBlock(const bcos::protocol::Block::ConstPtr& block, bool verify,
    std::function<void(const bcos::Error::ConstPtr&, bcos::protocol::BlockHeader::Ptr&&)>
        callback) noexcept
{
}

// by pbft & sync
void SchedulerImpl::commitBlock(const bcos::protocol::BlockHeader::ConstPtr& header,
    std::function<void(const bcos::Error::ConstPtr&)>) noexcept
{}

// by console, query committed committing executing
void SchedulerImpl::status(
    std::function<void(const Error::ConstPtr&, const bcos::protocol::Session::ConstPtr&)>
        callback) noexcept
{}

// by rpc
void SchedulerImpl::callTransaction(const protocol::Transaction::ConstPtr& tx,
    std::function<void(const Error::ConstPtr&, protocol::TransactionReceipt::Ptr&&)>) noexcept
{}

void SchedulerImpl::registerExecutor(
    std::function<void(const Error::ConstPtr&)> callback, std::string&& executorID) noexcept
{}

void SchedulerImpl::notifyCommitResult(const std::string_view& executorID,
    bcos::protocol::BlockNumber blockNumber,
    std::function<void(const Error::ConstPtr&)> callback) noexcept
{}

void SchedulerImpl::reset(std::function<void(const Error::ConstPtr&)> callback) noexcept {}