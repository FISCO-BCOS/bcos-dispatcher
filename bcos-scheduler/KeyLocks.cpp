#include "KeyLocks.h"
#include <assert.h>

using namespace bcos::executor;

bool KeyLocks::acquireKeyLock(
    const std::string_view& contract, const std::string_view& key, int contextID)
{
    assert(contextID >= 0);

    auto it = m_keyLocks.find(std::tuple{contract, key});
    if (it != m_keyLocks.end())
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
        m_keyLocks.emplace(std::string(contract), std::string(key), contextID);
        return true;
    }
}

std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> KeyLocks::getKeyLocksByContextID(
    int contextID) const
{
    std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> results;
    auto range = m_keyLocks.equal_range(contextID);

    for (auto it = range.first; it != range.second; ++it)
    {
        results.emplace_back(*it);
    }

    return results;
}

std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> KeyLocks::getKeyLocksByContract(
    const std::string_view& contract, int64_t excludeContextID) const
{
    std::vector<std::reference_wrapper<KeyLocks::KeyLockItem const>> results;
    auto range = m_keyLocks.equal_range(contract);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->contextID != excludeContextID)
        {
            results.emplace_back(*it);
        }
    }

    return results;
}

void KeyLocks::releaseKeyLocks(int contextID)
{
    auto range = m_keyLocks.equal_range(contextID);

    for (auto it = range.first; it != range.second; ++it)
    {
        m_keyLocks.erase(it);
    }
}