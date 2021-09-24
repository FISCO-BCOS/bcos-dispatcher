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

    m_batchStates.resize(m_block->transactionsMetaDataSize());
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
    size_t index = 0;
    for (auto& it : m_batchStates)
    {
        auto executor = m_executorManager->dispatchExecutor(it.message->to());

        executor->executeTransaction(
            std::move(it), [responseIt = std::move(responseIt)](bcos::Error::UniquePtr&& error,
                               bcos::protocol::ExecutionMessage::UniquePtr&& response) {
                if (error)
                {
                    // TODO: handle error
                }

                switch (response->type())
                {
                case protocol::ExecutionMessage::TXHASH:
                {
                    // TODO: handle error
                    break;
                }
                case bcos::protocol::ExecutionMessage::MESSAGE:
                {
                    break;
                }
                }
                *responseIt = std::move(response);
            })
    }
}