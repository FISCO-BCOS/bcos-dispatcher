#include "KeyLocks.h"
#include "Common.h"
#include <assert.h>
#include <bcos-framework/libutilities/DataConvertUtility.h>
#include <bcos-framework/libutilities/Error.h>
#include <boost/throw_exception.hpp>

using namespace bcos::scheduler;

bool KeyLocks::acquireKeyLock(
    const std::string_view& contract, const std::string_view& key, int64_t contextID, int64_t seq)
{
    SCHEDULER_LOG(TRACE) << "Acquire key lock, contract: " << contract << " key: " << toHex(key)
                         << " contextID: " << contextID << " seq: " << seq;

    auto it = m_keyLocks.get<1>().find(std::tuple{contract, key});
    if (it != m_keyLocks.get<1>().end())
    {
        if (it->contextID != contextID)
        {
            SCHEDULER_LOG(ERROR) << "Acquire key lock failed, request contextID: " << contextID
                                 << " expected contextID: " << it->contextID;

            // Another context is owing the key
            return false;
        }
    }

    SCHEDULER_LOG(TRACE) << "Acquire key lock success";

    // Current context owing the key, accquire it
    [[maybe_unused]] auto [insertedIt, inserted] = m_keyLocks.get<1>().emplace(
        KeyLockItem{std::string(contract), std::string(key), contextID, seq});

    if (!inserted)
    {
        SCHEDULER_LOG(ERROR) << "Insert key lock error!";
        return false;
    }

    return true;
}

std::vector<std::string> KeyLocks::getKeyLocksByContract(
    const std::string_view& contract, int64_t excludeContextID) const
{
    std::vector<std::string> results;
    auto count = m_keyLocks.get<2>().count(contract);

    if (count > 0)
    {
        auto range = m_keyLocks.get<2>().equal_range(contract);

        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->contextID != excludeContextID)
            {
                results.emplace_back(it->key);
            }
        }
    }

    return results;
}

void KeyLocks::releaseKeyLocks(int64_t contextID, int64_t seq)
{
    SCHEDULER_LOG(TRACE) << "Release key lock, contextID: " << contextID << " seq: " << seq;

    auto range = m_keyLocks.get<3>().equal_range(std::tuple{contextID, seq});

    for (auto it = range.first; it != range.second; ++it)
    {
        SCHEDULER_LOG(TRACE) << "Releasing key lock, contract: " << it->contract
                             << " key: " << toHex(it->key) << " contextID: " << it->contextID
                             << " seq: " << it->seq;
    }

    m_keyLocks.get<3>().erase(range.first, range.second);
}