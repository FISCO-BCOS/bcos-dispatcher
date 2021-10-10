#pragma once
#include "../bcos-scheduler/Common.h"
#include "MockExecutor.h"
#include <tuple>

namespace bcos::test
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
class MockParallelExecutor3 : public MockParallelExecutor
{
public:
    MockParallelExecutor3(const std::string& name) : MockParallelExecutor(name) {}

    void executeTransaction(bcos::protocol::ExecutionMessage::UniquePtr input,
        std::function<void(bcos::Error::UniquePtr&&, bcos::protocol::ExecutionMessage::UniquePtr&&)>
            callback) noexcept override
    {
        // Always success
        SCHEDULER_LOG(TRACE) << "Input:" << input.get() << " to:" << input->to();
        BOOST_CHECK(input);
        input->setStatus(0);
        input->setMessage("");

        std::string data = "Hello world!";
        input->setData(bcos::bytes(data.begin(), data.end()));
        input->setType(bcos::protocol::ExecutionMessage::FINISHED);

        auto inputShared =
            std::make_shared<bcos::protocol::ExecutionMessage::UniquePtr>(std::move(input));

        auto [it, inserted] = batchContracts.emplace(std::string((*inputShared)->to()),
            [callback, inputShared]() { callback(nullptr, std::move(*inputShared)); });

        (void)it;
        if (!inserted)
        {
            BOOST_FAIL("Duplicate contract: " + std::string((*inputShared)->to()));
        }

        if (batchContracts.size() == 3)
        {
            // current batch is finished
            std::map<std::string, std::function<void()>> results;
            results.swap(batchContracts);

            for (auto& it : results)
            {
                (it.second)();
            }
        }
    }

    std::map<std::string, std::function<void()>> batchContracts;
};
#pragma GCC diagnostic pop
}  // namespace bcos::test