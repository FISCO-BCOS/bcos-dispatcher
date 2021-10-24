#include "bcos-scheduler/ExecutorManager.h"
#include "bcos-scheduler/SchedulerImpl.h"
#include "interfaces/crypto/CryptoSuite.h"
#include "interfaces/crypto/KeyPairInterface.h"
#include "interfaces/executor/ExecutionMessage.h"
#include "interfaces/ledger/LedgerInterface.h"
#include "interfaces/protocol/BlockHeaderFactory.h"
#include "interfaces/protocol/ProtocolTypeDef.h"
#include "interfaces/protocol/TransactionReceipt.h"
#include "interfaces/protocol/TransactionReceiptFactory.h"
#include "interfaces/storage/StorageInterface.h"
#include "mock/MockExecutor.h"
#include "mock/MockExecutor3.h"
#include "mock/MockExecutorForCall.h"
#include "mock/MockExecutorForCreate.h"
#include "mock/MockLedger.h"
#include "mock/MockMultiParallelExecutor.h"
#include "mock/MockTransactionalStorage.h"
#include <bcos-framework/libexecutor/NativeExecutionMessage.h>
#include <bcos-framework/testutils/crypto/HashImpl.h>
#include <bcos-framework/testutils/crypto/SignatureImpl.h>
#include <bcos-tars-protocol/BlockFactoryImpl.h>
#include <bcos-tars-protocol/BlockHeaderFactoryImpl.h>
#include <bcos-tars-protocol/TransactionFactoryImpl.h>
#include <bcos-tars-protocol/TransactionMetaDataImpl.h>
#include <bcos-tars-protocol/TransactionReceiptFactoryImpl.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/test/unit_test.hpp>
#include <future>

namespace bcos::test
{
struct SchedulerFixture
{
    SchedulerFixture()
    {
        hashImpl = std::make_shared<Keccak256Hash>();
        signature = std::make_shared<Secp256k1SignatureImpl>();
        suite = std::make_shared<bcos::crypto::CryptoSuite>(hashImpl, signature, nullptr);

        ledger = std::make_shared<MockLedger>();
        executorManager = std::make_shared<scheduler::ExecutorManager>();
        storage = std::make_shared<MockTransactionalStorage>();

        auto stateStorage = std::make_shared<storage::StateStorage>(nullptr);
        storage->m_storage = stateStorage;

        transactionFactory = std::make_shared<bcostars::protocol::TransactionFactoryImpl>(suite);
        transactionReceiptFactory =
            std::make_shared<bcostars::protocol::TransactionReceiptFactoryImpl>(suite);
        executionMessageFactory = std::make_shared<bcos::executor::NativeExecutionMessageFactory>();

        blockFactory = std::make_shared<bcostars::protocol::BlockFactoryImpl>(
            suite, blockHeaderFactory, transactionFactory, transactionReceiptFactory);

        scheduler = std::make_shared<scheduler::SchedulerImpl>(
            executorManager, ledger, storage, executionMessageFactory, blockFactory, hashImpl);

        keyPair = suite->signatureImpl()->generateKeyPair();
    }

    ledger::LedgerInterface::Ptr ledger;
    scheduler::ExecutorManager::Ptr executorManager;
    std::shared_ptr<MockTransactionalStorage> storage;
    protocol::ExecutionMessageFactory::Ptr executionMessageFactory;
    protocol::TransactionReceiptFactory::Ptr transactionReceiptFactory;
    protocol::BlockHeaderFactory::Ptr blockHeaderFactory;
    bcos::crypto::Hash::Ptr hashImpl;
    scheduler::SchedulerImpl::Ptr scheduler;
    bcos::crypto::KeyPairInterface::Ptr keyPair;

    bcostars::protocol::TransactionFactoryImpl::Ptr transactionFactory;
    bcos::crypto::SignatureCrypto::Ptr signature;
    bcos::crypto::CryptoSuite::Ptr suite;
    bcostars::protocol::BlockFactoryImpl::Ptr blockFactory;
};

BOOST_FIXTURE_TEST_SUITE(Scheduler, SchedulerFixture)

BOOST_AUTO_TEST_CASE(executeBlock)
{
    // Add executor
    executorManager->addExecutor("executor1", std::make_shared<MockParallelExecutor3>("executor1"));

    // Generate a test block
    auto block = blockFactory->createBlock();
    block->blockHeader()->setNumber(100);

    for (size_t i = 0; i < 10; ++i)
    {
        auto metaTx =
            std::make_shared<bcostars::protocol::TransactionMetaDataImpl>(h256(i), "contract1");
        block->appendTransactionMetaData(std::move(metaTx));
    }

    for (size_t i = 10; i < 20; ++i)
    {
        auto metaTx =
            std::make_shared<bcostars::protocol::TransactionMetaDataImpl>(h256(i), "contract2");
        block->appendTransactionMetaData(std::move(metaTx));
    }

    for (size_t i = 20; i < 30; ++i)
    {
        auto metaTx =
            std::make_shared<bcostars::protocol::TransactionMetaDataImpl>(h256(i), "contract3");
        block->appendTransactionMetaData(std::move(metaTx));
    }

    bcos::protocol::BlockHeader::Ptr executedHeader;

    scheduler->executeBlock(
        block, false, [&](bcos::Error::Ptr&& error, bcos::protocol::BlockHeader::Ptr&& header) {
            BOOST_CHECK(!error);
            BOOST_CHECK(header);

            executedHeader = std::move(header);
        });

    BOOST_CHECK(executedHeader);
    BOOST_CHECK_NE(executedHeader->stateRoot(), h256());

    bcos::protocol::BlockNumber notifyBlockNumber = 0;
    scheduler->registerBlockNumberReceiver(
        [&](protocol::BlockNumber number) { notifyBlockNumber = number; });

    bool commited = false;
    scheduler->commitBlock(
        executedHeader, [&](bcos::Error::Ptr&& error, bcos::ledger::LedgerConfig::Ptr&& config) {
            commited = true;

            BOOST_CHECK(!error);
            BOOST_CHECK(config);
            BOOST_CHECK_EQUAL(config->blockTxCountLimit(), 100);
            BOOST_CHECK_EQUAL(config->consensusTimeout(), 200);
            BOOST_CHECK_EQUAL(config->leaderSwitchPeriod(), 300);
            BOOST_CHECK_EQUAL(config->consensusNodeList().size(), 1);
            BOOST_CHECK_EQUAL(config->observerNodeList().size(), 2);
            BOOST_CHECK_EQUAL(config->hash().hex(), h256(110).hex());
        });

    BOOST_CHECK(commited);
    BOOST_CHECK_EQUAL(notifyBlockNumber, 100);
}

BOOST_AUTO_TEST_CASE(parallelExecuteBlock)
{
    // Add executor
    executorManager->addExecutor(
        "executor1", std::make_shared<MockMultiParallelExecutor>("executor1"));

    // Generate a test block
    auto block = blockFactory->createBlock();
    block->blockHeader()->setNumber(100);

    for (size_t i = 0; i < 1000; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            auto metaTx = std::make_shared<bcostars::protocol::TransactionMetaDataImpl>(
                h256(i * j), "contract" + boost::lexical_cast<std::string>(j));
            block->appendTransactionMetaData(std::move(metaTx));
        }
    }

    std::promise<bcos::protocol::BlockHeader::Ptr> executedHeader;

    scheduler->executeBlock(
        block, false, [&](bcos::Error::Ptr error, bcos::protocol::BlockHeader::Ptr header) {
            BOOST_CHECK(!error);
            BOOST_CHECK(header);

            executedHeader.set_value(std::move(header));
        });

    auto header = executedHeader.get_future().get();

    BOOST_CHECK(header);
    BOOST_CHECK_NE(header->stateRoot(), h256());

    bcos::protocol::BlockNumber notifyBlockNumber = 0;
    scheduler->registerBlockNumberReceiver(
        [&](protocol::BlockNumber number) { notifyBlockNumber = number; });

    std::promise<void> commitPromise;
    scheduler->commitBlock(
        header, [&](bcos::Error::Ptr&& error, bcos::ledger::LedgerConfig::Ptr&& config) {
            BOOST_CHECK(!error);
            BOOST_CHECK(config);
            BOOST_CHECK_EQUAL(config->blockTxCountLimit(), 100);
            BOOST_CHECK_EQUAL(config->consensusTimeout(), 200);
            BOOST_CHECK_EQUAL(config->leaderSwitchPeriod(), 300);
            BOOST_CHECK_EQUAL(config->consensusNodeList().size(), 1);
            BOOST_CHECK_EQUAL(config->observerNodeList().size(), 2);
            BOOST_CHECK_EQUAL(config->hash().hex(), h256(110).hex());

            commitPromise.set_value();
        });

    BOOST_CHECK_EQUAL(notifyBlockNumber, 100);
}

BOOST_AUTO_TEST_CASE(call)
{
    // Add executor
    executorManager->addExecutor(
        "executor1", std::make_shared<MockParallelExecutorForCall>("executor1"));

    std::string inputStr = "Hello world! request";
    auto tx = blockFactory->transactionFactory()->createTransaction(0, "address_to",
        bytes(inputStr.begin(), inputStr.end()), 200, 300, "chain", "group", 500, keyPair);

    bcos::protocol::TransactionReceipt::Ptr receipt;

    scheduler->call(
        tx, [&](bcos::Error::Ptr error, bcos::protocol::TransactionReceipt::Ptr receiptResponse) {
            BOOST_CHECK(!error);
            BOOST_CHECK(receiptResponse);

            receipt = std::move(receiptResponse);
        });

    BOOST_CHECK_EQUAL(receipt->blockNumber(), 0);
    BOOST_CHECK_EQUAL(receipt->status(), 0);
    BOOST_CHECK_GT(receipt->gasUsed(), 0);
    auto output = receipt->output();

    std::string outputStr((char*)output.data(), output.size());
    BOOST_CHECK_EQUAL(outputStr, "Hello world! response");
}

BOOST_AUTO_TEST_CASE(registerExecutor)
{
    auto executor = std::make_shared<MockParallelExecutor>("executor1");
    auto executor2 = std::make_shared<MockParallelExecutor>("executor2");

    scheduler->registerExecutor(
        "executor1", executor, [&](Error::Ptr&& error) { BOOST_CHECK(!error); });
    scheduler->registerExecutor(
        "executor2", executor2, [&](Error::Ptr&& error) { BOOST_CHECK(!error); });
}

BOOST_AUTO_TEST_CASE(createContract)
{
    // Add executor
    executorManager->addExecutor(
        "executor1", std::make_shared<MockParallelExecutorForCreate>("executor1"));

    // Generate a test block
    auto block = blockFactory->createBlock();
    block->blockHeader()->setNumber(100);

    auto metaTx = std::make_shared<bcostars::protocol::TransactionMetaDataImpl>(h256(1), "");
    block->appendTransactionMetaData(std::move(metaTx));

    bcos::protocol::BlockHeader::Ptr executedHeader;
    scheduler->executeBlock(
        block, false, [&](bcos::Error::Ptr&& error, bcos::protocol::BlockHeader::Ptr&& header) {
            BOOST_CHECK(!error);
            BOOST_CHECK(header);

            executedHeader = std::move(header);
        });

    BOOST_CHECK(executedHeader);
    BOOST_CHECK_NE(executedHeader->stateRoot(), h256());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace bcos::test