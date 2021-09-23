#include "../bcos-dispatcher/ExecutorManager.h"
#include "interfaces/executor/ParallelTransactionExecutorInterface.h"
#include "libutilities/Common.h"
#include <boost/test/unit_test.hpp>
#include <memory>

namespace bcos::test
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
class FakeParallelExecutor : public bcos::executor::ParallelTransactionExecutorInterface
{
public:
    FakeParallelExecutor(const std::string& name) : m_name(name) {}

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

    /* ----- XA Transaction interface Start ----- */

    // Write data to storage uncommitted
    void prepare(const TwoPCParams& params,
        std::function<void(bcos::Error::Ptr&&)> callback) noexcept override
    {}

    // Commit uncommitted data
    void commit(const TwoPCParams& params,
        std::function<void(bcos::Error::Ptr&&)> callback) noexcept override
    {}

    // Rollback the changes
    void rollback(const TwoPCParams& params,
        std::function<void(bcos::Error::Ptr&&)> callback) noexcept override
    {}

    /* ----- XA Transaction interface End ----- */

    // drop all status
    void reset(std::function<void(bcos::Error::Ptr&&)> callback) noexcept override {}

    std::string m_name;
};
#pragma GCC diagnostic pop

struct ExecutorManagerFixture
{
    ExecutorManagerFixture() { executorManager = std::make_shared<dispatcher::ExecutorManager>(); }

    dispatcher::ExecutorManager::Ptr executorManager;
};

BOOST_FIXTURE_TEST_SUITE(TestExecutorManager, ExecutorManagerFixture)

BOOST_AUTO_TEST_CASE(addExecutor)
{
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("1", std::make_shared<FakeParallelExecutor>("1")));
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("2", std::make_shared<FakeParallelExecutor>("2")));
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("3", std::make_shared<FakeParallelExecutor>("3")));
    BOOST_CHECK_THROW(
        executorManager->addExecutor("3", std::make_shared<FakeParallelExecutor>("3")),
        bcos::Exception);
}

BOOST_AUTO_TEST_CASE(dispatch)
{
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("1", std::make_shared<FakeParallelExecutor>("1")));
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("2", std::make_shared<FakeParallelExecutor>("2")));
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("3", std::make_shared<FakeParallelExecutor>("3")));
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("4", std::make_shared<FakeParallelExecutor>("4")));

    std::vector<std::string> contracts;
    for (int i = 0; i < 100; ++i)
    {
        contracts.push_back(boost::lexical_cast<std::string>(i));
    }

    auto executors = executorManager->dispatchExecutor(contracts.begin(), contracts.end());
    BOOST_CHECK_EQUAL(executors.size(), 100);

    std::map<std::string, int> executor2count{
        std::pair("1", 0), std::pair("2", 0), std::pair("3", 0), std::pair("4", 0)};
    for (auto it = executors.begin(); it != executors.end(); ++it)
    {
        ++executor2count[std::dynamic_pointer_cast<FakeParallelExecutor>(*it)->name()];
    }

    BOOST_CHECK_EQUAL(executor2count["1"], 25);
    BOOST_CHECK_EQUAL(executor2count["2"], 25);
    BOOST_CHECK_EQUAL(executor2count["3"], 25);
    BOOST_CHECK_EQUAL(executor2count["4"], 25);

    auto executors2 = executorManager->dispatchExecutor(contracts.begin(), contracts.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(
        executors.begin(), executors.end(), executors2.begin(), executors2.end());

    std::vector<std::string> contracts2;
    for (int i = 0; i < 40; ++i)
    {
        contracts2.push_back(boost::lexical_cast<std::string>(i + 1000));
    }

    contracts2.insert(contracts2.end(), contracts.begin(), contracts.end());

    auto executors3 = executorManager->dispatchExecutor(contracts2.begin(), contracts2.end());
    std::map<std::string, int> executor2count2{
        std::pair("1", 0), std::pair("2", 0), std::pair("3", 0), std::pair("4", 0)};
    for (auto it = executors3.begin(); it != executors3.end(); ++it)
    {
        ++executor2count2[std::dynamic_pointer_cast<FakeParallelExecutor>(*it)->name()];
    }

    BOOST_CHECK_EQUAL(executor2count2["1"], 35);
    BOOST_CHECK_EQUAL(executor2count2["2"], 35);
    BOOST_CHECK_EQUAL(executor2count2["3"], 35);
    BOOST_CHECK_EQUAL(executor2count2["4"], 35);

    auto executors4 = executorManager->dispatchExecutor(contracts2.begin(), contracts2.end());

    std::map<std::string, executor::ParallelTransactionExecutorInterface::Ptr> contract2executor;
    for (size_t i = 0; i < contracts2.size(); ++i)
    {
        contract2executor.insert({contracts2[i], executors4[i]});
    }

    BOOST_CHECK_EQUAL_COLLECTIONS(
        executors3.begin(), executors3.end(), executors4.begin(), executors4.end());

    // record executor3's contract
    std::set<std::string> contractsInExecutor3;
    for (size_t i = 0; i < executors4.size(); ++i)
    {
        auto executor = executors4[i];
        if (std::dynamic_pointer_cast<FakeParallelExecutor>(executor)->name() == "3")
        {
            contractsInExecutor3.insert(contracts2[i]);
        }
    }

    BOOST_CHECK_EQUAL(contractsInExecutor3.size(), 35);

    BOOST_CHECK_NO_THROW(executorManager->removeExecutor("3"));

    std::vector<std::string> contracts3;
    for (int i = 0; i < 10; ++i)
    {
        contracts2.push_back(boost::lexical_cast<std::string>(i + 2000));
    }

    contracts2.insert(contracts2.end(), contracts3.begin(), contracts3.end());
    auto executors5 = executorManager->dispatchExecutor(contracts2.begin(), contracts2.end());
    BOOST_CHECK_EQUAL(executors5.size(), 150);

    size_t oldContract = 0;
    for (size_t i = 0; i < contracts2.size(); ++i)
    {
        auto contract = contracts2[i];
        if (boost::lexical_cast<int>(contract) < 2000 &&
            contractsInExecutor3.find(contract) == contractsInExecutor3.end())
        {
            ++oldContract;

            BOOST_CHECK_EQUAL(executors5[i], contract2executor[contract]);
        }
    }

    BOOST_CHECK_EQUAL(oldContract, 150 - 35 - 10);  // exclude new contract and executor3's contract
}

BOOST_AUTO_TEST_CASE(remove)
{
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("1", std::make_shared<FakeParallelExecutor>("1")));
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("2", std::make_shared<FakeParallelExecutor>("2")));
    BOOST_CHECK_NO_THROW(
        executorManager->addExecutor("3", std::make_shared<FakeParallelExecutor>("3")));
    BOOST_CHECK_THROW(
        executorManager->addExecutor("3", std::make_shared<FakeParallelExecutor>("3")),
        bcos::Exception);

    BOOST_CHECK_NO_THROW(executorManager->removeExecutor("2"));
    BOOST_CHECK_THROW(executorManager->removeExecutor("10"), bcos::Exception);
    BOOST_CHECK_THROW(executorManager->removeExecutor("2"), bcos::Exception);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace bcos::test