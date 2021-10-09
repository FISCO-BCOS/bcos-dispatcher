#pragma once
#include "interfaces/executor/ParallelTransactionExecutorInterface.h"

namespace bcos::test
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
class MockParallelExecutor : public bcos::executor::ParallelTransactionExecutorInterface
{
public:
    MockParallelExecutor(const std::string& name) : m_name(name) {}

    const std::string& name() const { return m_name; }

    void nextBlockHeader(const bcos::protocol::BlockHeader::ConstPtr& blockHeader,
        std::function<void(bcos::Error::UniquePtr&&)> callback) noexcept override
    {}

    void executeTransaction(bcos::protocol::ExecutionMessage::UniquePtr input,
        std::function<void(bcos::Error::UniquePtr&&, bcos::protocol::ExecutionMessage::UniquePtr&&)>
            callback) noexcept override
    {}

    void dagExecuteTransactions(
        const gsl::span<bcos::protocol::ExecutionMessage::UniquePtr>& inputs,
        std::function<void(
            bcos::Error::UniquePtr&&, std::vector<bcos::protocol::ExecutionMessage::UniquePtr>&&)>
            callback) noexcept override
    {}

    void call(bcos::protocol::ExecutionMessage::UniquePtr input,
        std::function<void(bcos::Error::UniquePtr&&, bcos::protocol::ExecutionMessage::UniquePtr&&)>
            callback) noexcept override
    {}

    void getTableHashes(bcos::protocol::BlockNumber number,
        std::function<void(
            bcos::Error::UniquePtr&&, std::vector<std::tuple<std::string, crypto::HashType>>&&)>
            callback) noexcept override
    {}

    void prepare(const TwoPCParams& params,
        std::function<void(bcos::Error::Ptr&&)> callback) noexcept override
    {}

    void commit(const TwoPCParams& params,
        std::function<void(bcos::Error::Ptr&&)> callback) noexcept override
    {}

    void rollback(const TwoPCParams& params,
        std::function<void(bcos::Error::Ptr&&)> callback) noexcept override
    {}

    void reset(std::function<void(bcos::Error::Ptr&&)> callback) noexcept override {}

    std::string m_name;
};
#pragma GCC diagnostic pop
}  // namespace bcos::test
