#pragma once

#include "bcos-framework/libutilities/Error.h"
#include "interfaces/protocol/Block.h"
#include "interfaces/protocol/BlockHeader.h"
#include "interfaces/protocol/ProtocolTypeDef.h"

namespace bcos::scheduler
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

    BlockContext(bcos::protocol::Block::ConstPtr block) : m_block(std::move(block)) {}

    void asyncExecute(std::function<void(Error::UniquePtr&&)> callback) noexcept;

    bcos::protocol::BlockNumber number() { return m_block->blockHeaderConst()->number(); }

    void startExecute(std::function<void(bcos::Error::UniquePtr&&)> callback);

    Status status() { return m_status; }
    void setStatus(Status status) { m_status = status; }

private:
    bcos::protocol::Block::ConstPtr m_block;
    Status m_status = IDLE;
};
}  // namespace bcos::scheduler