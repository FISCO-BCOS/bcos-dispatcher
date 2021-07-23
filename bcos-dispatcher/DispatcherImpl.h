#pragma once

#include "bcos-framework/interfaces/dispatcher/DispatcherInterface.h"
#include "bcos-framework/interfaces/protocol/ProtocolTypeDef.h"
#include <bcos-framework/interfaces/txpool/TxPoolInterface.h>
#include <tbb/mutex.h>
#include <unordered_map>

#define DISPATCHER_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("Dispatcher")

namespace bcos
{
namespace dispatcher
{
class DispatcherImpl : public DispatcherInterface
{
public:
    using Ptr = std::shared_ptr<DispatcherImpl>;
    using ExecutionResultCallback =
        std::function<void(const Error::Ptr&, const protocol::BlockHeader::Ptr&)>;
    DispatcherImpl() = default;
    ~DispatcherImpl() override {}
    void asyncExecuteBlock(const protocol::Block::Ptr& _block, bool _verify,
        ExecutionResultCallback _callback) override;
    void asyncGetLatestBlock(
        std::function<void(const Error::Ptr&, const protocol::Block::Ptr&)> _callback) override;

    void asyncNotifyExecutionResult(const Error::Ptr& _error,
        bcos::crypto::HashType const& _orgHash, const protocol::BlockHeader::Ptr& _header,
        std::function<void(const Error::Ptr&)> _callback) override;

    void init(bcos::txpool::TxPoolInterface::Ptr _txpool) { m_txpool = _txpool; }
    void start() override;
    void stop() override;

    virtual void asyncExecuteCompletedBlock(
        const protocol::Block::Ptr& _block, bool _verify, ExecutionResultCallback _callback);

private:
    bcos::txpool::TxPoolInterface::Ptr m_txpool;

    // increase order
    struct BlockCmp
    {
        bool operator()(
            protocol::Block::Ptr const& _first, protocol::Block::Ptr const& _second) const
        {
            // increase order
            return _first->blockHeader()->number() > _second->blockHeader()->number();
        }
    };
    std::priority_queue<protocol::Block::Ptr, std::vector<protocol::Block::Ptr>, BlockCmp>
        m_blockQueue;
    std::map<bcos::crypto::HashType, std::vector<ExecutionResultCallback>> m_callbackMap;

    std::queue<std::function<void(const Error::Ptr&, const protocol::Block::Ptr&)>> m_waitingQueue;
    mutable SharedMutex x_blockQueue;
};
}  // namespace dispatcher
}  // namespace bcos