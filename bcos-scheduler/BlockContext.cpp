#include "BlockContext.h"
#include "bcos-scheduler/Common.h"
#include "interfaces/executor/ExecutionMessage.h"
#include <boost/algorithm/hex.hpp>
#include <atomic>
#include <iterator>

using namespace bcos::scheduler;

void BlockContext::asyncExecute(std::function<void(Error::UniquePtr&&)> callback) noexcept
{
    if (m_status != IDLE)
    {
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::WrongStatus, "Wrong status"));
        return;
    }

    m_receipts.resize(m_block->transactionsMetaDataSize());
    for (size_t i = 0; i < m_block->transactionsMetaDataSize(); ++i)
    {
        auto metaData = m_block->transactionMetaData(i);

        auto message = m_executionMessageFactory->createExecutionMessage();
        message->setType(protocol::ExecutionMessage::TXHASH);
        message->setTransactionHash(metaData->hash());

        message->setTo(std::string(metaData->to()));  // Always readable

        m_executiveStates.emplace_back(i);
        m_executiveStates.back().message.swap(message);
    }

    // runBatch();

    callback(nullptr);
}

void BlockContext::runBatch(std::function<void(Error::UniquePtr&&)> callback)
{
    struct BatchStatus
    {
        std::atomic_bool allSended = false;
        std::atomic_size_t total = 0;
        std::atomic_size_t received = 0;

        std::atomic_bool callbackExecuted = false;
        std::function<void(Error::UniquePtr&&)> callback;
    };

    auto batchStatus = std::make_shared<BatchStatus>();
    batchStatus->callback = std::move(callback);

    for (auto it = m_executiveStates.begin();; ++it)
    {
        if (it == m_executiveStates.end())
        {
            batchStatus->allSended = true;
            break;
        }

        if (m_calledContract.find(it->message->to()) != m_calledContract.end())
        {
            continue;  // Another context processing
        }

        m_calledContract.emplace(it->message->to());
        switch (it->message->type())
        {
        // Request type, push stack
        case protocol::ExecutionMessage::TXHASH:
        case protocol::ExecutionMessage::MESSAGE:
        {
            auto seq = it->m_currentSeq++;

            it->callStack.push(seq);
            it->callHistory.push_front(seq);

            it->message->setSeq(seq);

            break;
        }
        // Return type, pop stack
        case protocol::ExecutionMessage::FINISHED:
        case protocol::ExecutionMessage::REVERT:
        {
            it->callStack.pop();

            // Empty stack, execution is finished
            if (it->callStack.empty())
            {
                // TODO: Execution is finished, generate receipt
                // m_receipts[it->contextID] = m_transactionReceiptFactory->createReceipt();
                it = m_executiveStates.erase(it);
                continue;
            }

            it->message->setSeq(it->callStack.top());

            break;
        }
        // Retry type, send again
        case protocol::ExecutionMessage::WAIT_KEY:
        {
            auto keyIt = m_keyLocks.find(it->message->keyLocks()[0]);
            if (keyIt != m_keyLocks.end() && keyIt->second.count > 0 &&
                keyIt->second.contextID != it->contextID)
            {
                continue;  // Request key is locking
            }

            break;
        }
        // Retry type, send again
        case protocol::ExecutionMessage::SEND_BACK:
        {
            it->message->setType(protocol::ExecutionMessage::TXHASH);
            break;
        }
        }

        ++batchStatus->total;
        auto executor = m_executorManager->dispatchExecutor(it->message->to());
        executor->executeTransaction(
            std::move(it->message), [it, batchStatus](bcos::Error::UniquePtr&& error,
                                        bcos::protocol::ExecutionMessage::UniquePtr&& response) {
                if (error)
                {
                }

                it->message = std::move(response);

            });
    }
}