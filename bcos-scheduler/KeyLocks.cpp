#include "KeyLocks.h"
#include <assert.h>

using namespace bcos::executor;

bool KeyLocks::acquireKeyLock(
    const std::string_view& contract, const std::string_view& key, int contextID)
{
    assert(contextID >= 0);

    auto it = m_keyLocks.get<0>().find(std::tuple{contract, key});
    if (it != m_keyLocks.get<0>().end())
    {
        if (it->contextID == contextID)
        {
            // Current context owing the key
            return true;
        }
        else
        {
            // Another context is owing the key
            return false;
        }
    }
    else
    {
        // No context owing the key, accquire it
        m_keyLocks.emplace(KeyLockItem{std::string(contract), std::string(key), contextID});
        return true;
    }
}

std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> KeyLocks::getKeyLocksByContextID(
    [[maybe_unused]] int64_t contextID) const
{
    std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> results;
    auto range = m_keyLocks.get<2>().equal_range(contextID);

    for (auto it = range.first; it != range.second; ++it)
    {
        results.emplace_back(*it);
    }

    return results;
}

std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> KeyLocks::getKeyLocksByContract(
    [[maybe_unused]] const std::string_view& contract,
    [[maybe_unused]] int64_t excludeContextID) const
{
    std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> results;
    auto range = m_keyLocks.get<1>().equal_range(contract);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->contextID != excludeContextID)
        {
            results.emplace_back(*it);
        }
    }

    return results;
}

void KeyLocks::releaseKeyLocks([[maybe_unused]] int64_t contextID)
{
    auto range = m_keyLocks.get<2>().equal_range(contextID);

    for (auto it = range.first; it != range.second; ++it)
    {
        m_keyLocks.get<2>().erase(it);
    }
}