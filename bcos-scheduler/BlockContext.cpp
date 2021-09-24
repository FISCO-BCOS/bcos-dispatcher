#include "BlockContext.h"

using namespace bcos::scheduler;

void BlockContext::asyncExecute(std::function<void(Error::UniquePtr&&)> callback) noexcept
{
    callback(nullptr);
}