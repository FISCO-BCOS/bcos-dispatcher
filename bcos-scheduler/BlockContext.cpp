#include "BlockContext.h"
#include "bcos-scheduler/Common.h"
#include "interfaces/executor/ExecutionMessage.h"
#include <boost/algorithm/hex.hpp>
#include <iterator>

using namespace bcos::scheduler;

void BlockContext::asyncExecute(std::function<void(Error::UniquePtr&&)> callback) noexcept
{
    if (m_status != IDLE)
    {
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::WrongStatus, "Wrong status"));
        return;
    }

    for (size_t i = 0; i < m_block->transactionsMetaDataSize(); ++i)
    {
        auto metaData = m_block->transactionMetaData(i);

        auto message = m_executionMessageFactory->createExecutionMessage();
        message->setType(protocol::ExecutionMessage::TXHASH);
        message->setTransactionHash(metaData->hash());

        std::string to;
        to.reserve(40);
        boost::algorithm::hex_lower(
            metaData->hash().begin(), metaData->hash().end(), std::back_insert_iterator(to));
        message->setTo(std::move(to));

        m_batchStates[i].message.swap(message);
    }

    runBatch();

    callback(nullptr);
}

void BlockContext::runBatch(std::function<void(Error::UniquePtr&&)> callback)
{
    for (auto it = m_batchStates.begin(); it != m_batchStates.end(); ++it)
    {
        if (m_calledContract.find(it->message->to()) != m_calledContract.end())
        {
            continue;  // Another context processing
        }

        m_calledContract.emplace(it->message->to());
        switch (it->message->type())
        {
        case protocol::ExecutionMessage::TXHASH:
        case protocol::ExecutionMessage::MESSAGE:
        {
            auto seq = it->m_currentSeq++;

            it->callStack.push(seq);
            it->callHistory.push_front(seq);
            break;
        }
        case protocol::ExecutionMessage::FINISHED:
        case protocol::ExecutionMessage::REVERT:
        {
            it->callStack.pop();

            if (it->callStack.empty())
            {
                // TODO: Execution is finished, generate receipt
                // m_receipts[it->contextID] = m_transactionReceiptFactory->createReceipt();
                it = m_batchStates.erase(it);
                continue;
            }

            break;
        }
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
        case protocol::ExecutionMessage::SEND_BACK:
        {
            it->message->setType(protocol::ExecutionMessage::TXHASH);
            break;
        }
        }

        auto executor = m_executorManager->dispatchExecutor(it->message->to());
        executor->executeTransaction(
            std::move(it->message), [this, it](bcos::Error::UniquePtr&& error,
                                        bcos::protocol::ExecutionMessage::UniquePtr&& response) {
                if (error)
                {
                    // TODO: handle error
                }

                it->message = std::move(response);

                // switch (response->type())
                // {
                // case protocol::ExecutionMessage::TXHASH:
                // {
                //     // TODO: handle error
                //     break;
                // }
                // case bcos::protocol::ExecutionMessage::MESSAGE:
                // {
                //     break;
                // }
                // }
                // *responseIt = std::move(response);
            });
    }
}