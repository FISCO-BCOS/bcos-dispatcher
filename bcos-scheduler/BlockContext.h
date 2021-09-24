#pragma once

#include "interfaces/protocol/Block.h"
#include "interfaces/protocol/BlockHeader.h"
#include "interfaces/protocol/ProtocolTypeDef.h"

namespace bcos::dispatcher
{
class BlockContext
{
public:
    using Ptr = std::shared_ptr<BlockContext>;
    using ConstPtr = std::shared_ptr<const BlockContext>;

    enum Status : int8_t
    {
        IDLE,
        EXECUTING,
        FINISHED,
    };

    BlockContext(bcos::protocol::Block::ConstPtr block,
        std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)> callback)
      : m_block(std::move(block)), m_callback(std::move(callback))
    {}

    void startExecute();

    bcos::protocol::BlockNumber number() { return m_block->blockHeaderConst()->number(); }

    Status status() { return m_status; }
    void setStatus(Status status) { m_status = status; }

private:
    bcos::protocol::Block::ConstPtr m_block;
    std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)> m_callback;
    Status m_status = IDLE;
};
}  // namespace bcos::dispatcher