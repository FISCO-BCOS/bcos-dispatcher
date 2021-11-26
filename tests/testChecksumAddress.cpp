#include "bcos-scheduler/ExecutorManager.h"
#include "bcos-scheduler/SchedulerImpl.h"
#include "bcos-scheduler/ChecksumAddress.h"
#include "interfaces/crypto/CryptoSuite.h"
#include "interfaces/crypto/KeyPairInterface.h"
#include "interfaces/executor/ExecutionMessage.h"
#include "interfaces/ledger/LedgerInterface.h"
#include "interfaces/protocol/BlockHeaderFactory.h"
#include "interfaces/protocol/ProtocolTypeDef.h"
#include "interfaces/protocol/Transaction.h"
#include "interfaces/protocol/TransactionReceipt.h"
#include "interfaces/protocol/TransactionReceiptFactory.h"
#include "interfaces/protocol/TransactionSubmitResult.h"
#include "interfaces/storage/StorageInterface.h"
#include "libprotocol/TransactionSubmitResultFactoryImpl.h"
#include "mock/MockExecutor.h"
#include "mock/MockExecutor3.h"
#include "mock/MockExecutorForCall.h"
#include "mock/MockExecutorForCreate.h"
#include "mock/MockExecutorForMessageDAG.h"
#include "mock/MockLedger.h"
#include "mock/MockMultiParallelExecutor.h"
#include "mock/MockRPC.h"
#include "mock/MockTransactionalStorage.h"
#include <bcos-framework/interfaces/executor/PrecompiledTypeDef.h>
#include <bcos-framework/libexecutor/NativeExecutionMessage.h>
#include <bcos-framework/testutils/crypto/HashImpl.h>
#include <bcos-framework/testutils/crypto/SignatureImpl.h>
#include <bcos-tars-protocol/protocol/BlockFactoryImpl.h>
#include <bcos-tars-protocol/protocol/BlockHeaderFactoryImpl.h>
#include <bcos-tars-protocol/protocol/TransactionFactoryImpl.h>
#include <bcos-tars-protocol/protocol/TransactionMetaDataImpl.h>
#include <bcos-tars-protocol/protocol/TransactionReceiptFactoryImpl.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/latch.hpp>
#include <future>
#include <memory>

namespace bcos::test
{
struct SchedulerFixture
{
    SchedulerFixture()
    {
        hashImpl = std::make_shared<Keccak256Hash>();
    }

    bcos::crypto::Hash::Ptr hashImpl;

    void check(const std::string& addr)
    {
        auto ret = addr;
        toChecksumAddress(ret, hashImpl);
        BOOST_CHECK_EQUAL(addr, ret);
    }

};

BOOST_FIXTURE_TEST_SUITE(utils, SchedulerFixture)

BOOST_AUTO_TEST_CASE(checksumAddress)
{
    check("5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed");
    check("fB6916095ca1df60bB79Ce92cE3Ea74c37c5d359");
    check("dbF03B407c01E7cD3CBea99509d93f8DDDC8C6FB");
    check("D1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb");
    check("fB6916095ca1df60bB79Ce92cE3Ea74c37c5d359");
    check("3C589CB0Be25f651b0563e052DEa63d3844C33e6");
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace bcos::test