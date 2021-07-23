#include "DispatcherImpl.h"
#include <thread>

using namespace bcos::dispatcher;
using namespace bcos::crypto;
using namespace bcos::protocol;

void DispatcherImpl::asyncExecuteBlock(
    const Block::Ptr& _block, bool _verify, ExecutionResultCallback _callback)
{
    // with completed block
    if (_verify)
    {
        asyncExecuteCompletedBlock(_block, _verify, _callback);
        return;
    }
    // with only txsHash
    auto txsHashList = std::make_shared<HashList>();
    for (size_t i = 0; i < _block->transactionsHashSize(); i++)
    {
        txsHashList->emplace_back(_block->transactionHash(i));
    }
    auto self = std::weak_ptr<DispatcherInterface>(shared_from_this());
    m_txpool->asyncFillBlock(
        txsHashList, [self, _block, _verify, _callback](Error::Ptr _error, TransactionsPtr _txs) {
            if (_error)
            {
                DISPATCHER_LOG(ERROR)
                    << LOG_DESC("asyncExecuteBlock: asyncFillBlock failed")
                    << LOG_KV("consNum", _block->blockHeader()->number())
                    << LOG_KV("hash", _block->blockHeader()->hash().abridged())
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage());
                _callback(_error, nullptr);
                return;
            }
            try
            {
                auto dispatcher = self.lock();
                if (!dispatcher)
                {
                    _callback(std::make_shared<Error>(-1, "internal error"), nullptr);
                    return;
                }
                // fill the block
                for (auto tx : *_txs)
                {
                    _block->appendTransaction(tx);
                }
                // calculate the txsRoot(TODO: async here to optimize the performance)
                _block->calculateTransactionRoot(true);
                auto dispatcherImpl = std::dynamic_pointer_cast<DispatcherImpl>(dispatcher);
                dispatcherImpl->asyncExecuteCompletedBlock(_block, _verify, _callback);
            }
            catch (std::exception const& e)
            {
                DISPATCHER_LOG(WARNING) << LOG_DESC("asyncExecuteBlock exception")
                                        << LOG_KV("error", boost::diagnostic_information(e))
                                        << LOG_KV("consNum", _block->blockHeader()->number())
                                        << LOG_KV("hash", _block->blockHeader()->hash().abridged());
                _callback(std::make_shared<Error>(-1, "internal error"), nullptr);
            }
        });
}

void DispatcherImpl::asyncExecuteCompletedBlock(
    const Block::Ptr& _block, bool _verify, ExecutionResultCallback _callback)
{
    // Note: the waiting queue must be exist to accelerate the blocks-fetching speed
    std::list<std::function<void()>> callbacks;
    {
        WriteGuard l(x_blockQueue);
        m_blockQueue.push(_block);
        auto hash = _block->blockHeader()->hash();
        if (m_callbackMap.count(hash))
        {
            m_callbackMap[hash].push_back(_callback);
        }
        else
        {
            std::vector<ExecutionResultCallback> callbackList;
            m_callbackMap[hash] = callbackList;
            m_callbackMap[hash].push_back(_callback);
        }
        DISPATCHER_LOG(INFO) << LOG_DESC("asyncExecuteCompletedBlock")
                             << LOG_KV("consNum", _block->blockHeader()->number())
                             << LOG_KV("hash", _block->blockHeader()->hash().abridged())
                             << LOG_KV("queueSize", m_blockQueue.size())
                             << LOG_KV("verify", _verify);
        while (!m_waitingQueue.empty() && !m_blockQueue.empty())
        {
            auto callback = m_waitingQueue.front();
            m_waitingQueue.pop();
            auto block = m_blockQueue.top();
            m_blockQueue.pop();
            DISPATCHER_LOG(INFO) << LOG_DESC("asyncGetLatestBlock: dispatch block")
                                 << LOG_KV("consNum", block->blockHeader()->number())
                                 << LOG_KV("hash", block->blockHeader()->hash().abridged());
            callbacks.push_back([callback, block]() { callback(nullptr, block); });
        }
    }
    for (auto callback : callbacks)
    {
        callback();
    }
}

void DispatcherImpl::asyncGetLatestBlock(
    std::function<void(const Error::Ptr&, const Block::Ptr&)> _callback)
{
    Block::Ptr _obtainedBlock = nullptr;
    {
        WriteGuard l(x_blockQueue);
        // get pending block to execute
        if (!m_blockQueue.empty())
        {
            _obtainedBlock = m_blockQueue.top();
            m_blockQueue.pop();
            DISPATCHER_LOG(INFO) << LOG_DESC("asyncGetLatestBlock: dispatch block")
                                 << LOG_KV("consNum", _obtainedBlock->blockHeader()->number())
                                 << LOG_KV(
                                        "hash", _obtainedBlock->blockHeader()->hash().abridged());
        }
        else
        {
            m_waitingQueue.emplace(_callback);
        }
    }
    if (_obtainedBlock)
    {
        _callback(nullptr, _obtainedBlock);
    }
}

void DispatcherImpl::asyncNotifyExecutionResult(const Error::Ptr& _error,
    bcos::crypto::HashType const& _orgHash, const BlockHeader::Ptr& _header,
    std::function<void(const Error::Ptr&)> _callback)
{
    std::vector<ExecutionResultCallback> callbackList;
    {
        WriteGuard l(x_blockQueue);
        if (!m_callbackMap.count(_orgHash))
        {
            auto error = std::make_shared<bcos::Error>(
                -1, "No such block: " + boost::lexical_cast<std::string>(_header->number()));
            _callback(error);
            return;
        }
        callbackList = m_callbackMap[_orgHash];
        m_callbackMap.erase(_orgHash);
    }
    // Note: must call the callback after the lock, in case of the callback retry to call
    // asyncExecuteBlock
    for (auto callback : callbackList)
    {
        callback(_error, _header);
    }
    DISPATCHER_LOG(INFO) << LOG_DESC("asyncNotifyExecutionResult")
                         << LOG_KV("consNum", _header->number())
                         << LOG_KV("orgHash", _orgHash.abridged())
                         << LOG_KV("hashAfterExec", _header->hash().abridged());
    // notify success to the executor
    _callback(nullptr);
}

void DispatcherImpl::start() {}

void DispatcherImpl::stop() {}