#include "BlockExecutive.h"
#include "bcos-scheduler/Common.h"
#include "interfaces/executor/ExecutionMessage.h"
#include "libutilities/Error.h"
#include <boost/algorithm/hex.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <atomic>
#include <iterator>

using namespace bcos::scheduler;

void BlockExecutive::asyncExecute(
    std::function<void(Error::UniquePtr&&, protocol::BlockHeader::Ptr)> callback) noexcept
{
    if (m_status != IDLE)
    {
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, "Invalid status"), nullptr);
        return;
    }

    // TODO: 判断是否是MetaData还是TxData
    m_executiveResults.resize(m_block->transactionsMetaDataSize());
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

    class BatchCallback : public std::enable_shared_from_this<BatchCallback>
    {
    public:
        BatchCallback(BlockExecutive* self,
            std::function<void(Error::UniquePtr&&, protocol::BlockHeader::Ptr)> callback)
          : m_self(self), m_callback(std::move(callback))
        {}

        void operator()(Error::UniquePtr&& error) const
        {
            if (error)
            {
                m_callback(
                    BCOS_ERROR_WITH_PREV_UNIQUE_PTR(-1, "Execute with errors", *error), nullptr);
                return;
            }

            if (!m_self->m_executiveStates.empty())
            {
                m_self->startBatch(std::bind(
                    &BatchCallback::operator(), shared_from_this(), std::placeholders::_1));
            }
            else
            {
                // All Transaction finished
                auto header = m_self->generateResultBlockHeader();

                m_callback(nullptr, std::move(header));
            }
        }

    private:
        BlockExecutive* m_self;
        std::function<void(Error::UniquePtr&&, protocol::BlockHeader::Ptr)> m_callback;
    };

    auto batchCallback = std::make_shared<BatchCallback>(this, std::move(callback));

    startBatch(std::bind(&BatchCallback::operator(), batchCallback, std::placeholders::_1));
}

void BlockExecutive::startBatch(std::function<void(Error::UniquePtr&&)> callback)
{
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
            auto seq = it->currentSeq++;

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
                // Execution is finished, generate receipt
                m_executiveResults[it->contextID].receipt =
                    m_transactionReceiptFactory->createReceipt(it->message->gasAvailable(),
                        it->message->to(),
                        std::make_shared<std::vector<bcos::protocol::LogEntry>>(
                            std::move(it->message->takeLogEntries())),
                        it->message->status(), std::move(it->message->takeData()),
                        m_block->blockHeaderConst()->number());
                it = m_executiveStates.erase(it);
                continue;
            }

            it->message->setSeq(it->callStack.top());

            break;
        }
        // Retry type, send again
        case protocol::ExecutionMessage::WAIT_KEY:
        {
            auto keyIt = m_keyLocks.find(
                std::string(it->message->keyLockAcquired()));  // TODO: replace tbb with onetbb,
                                                               // support find with string_view
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
            std::move(it->message), [this, it, batchStatus](bcos::Error::UniquePtr&& error,
                                        bcos::protocol::ExecutionMessage::UniquePtr&& response) {
                ++batchStatus->received;

                if (error)
                {
                    SCHEDULER_LOG(ERROR)
                        << "Execute transaction error: " << boost::diagnostic_information(*error);

                    it->error = std::move(error);
                    m_executiveStates.erase(it);
                }
                else
                {
                    it->message = std::move(response);
                }

                checkBatch(*batchStatus);
            });
    }

    checkBatch(*batchStatus);
}

void BlockExecutive::checkBatch(BatchStatus& status)
{
    if (status.allSended && status.received == status.total)
    {
        bool expect = false;
        if (status.callbackExecuted.compare_exchange_strong(expect, true))  // Run callback once
        {
            size_t errorCount = 0;
            size_t successCount = 0;

            for (auto& it : m_executiveStates)
            {
                if (it.error)
                {
                    // with errors
                    ++errorCount;
                    SCHEDULER_LOG(ERROR)
                        << "Batch with error: " << boost::diagnostic_information(*it.error);
                }
                else
                {
                    ++successCount;
                }
            }

            SCHEDULER_LOG(INFO) << "Batch run success"
                                << " total: " << errorCount + successCount
                                << " success: " << successCount << " error: " << errorCount;

            if (errorCount > 0)
            {
                status.callback(BCOS_ERROR_UNIQUE_PTR(-1, "Batch with errors"));
                return;
            }

            status.callback(nullptr);
        }
    }
}

bcos::protocol::BlockHeader::Ptr BlockExecutive::generateResultBlockHeader()
{
    auto blockHeader =
        m_blockHeaderFactory->createBlockHeader(m_block->blockHeaderConst()->number());

    // blockHeader->setStateRoot(const bcos::crypto::HashType &_stateRoot);

    return blockHeader;
}