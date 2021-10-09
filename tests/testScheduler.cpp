#include "bcos-scheduler/SchedulerImpl.h"
#include <bcos-framework/testutils/crypto/HashImpl.h>
#include <boost/test/unit_test.hpp>

namespace bcos::test
{
struct SchedulerFixture
{
    SchedulerFixture() { hashImpl = std::make_shared<Keccak256Hash>(); }

    scheduler::SchedulerImpl::Ptr scheduler;
    bcos::crypto::Hash::Ptr hashImpl;
};

BOOST_FIXTURE_TEST_SUITE(Scheduler, SchedulerFixture)

BOOST_AUTO_TEST_CASE(addExecutor) {}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace bcos::test