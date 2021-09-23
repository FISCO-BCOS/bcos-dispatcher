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

    enum Status
    {
        EXECUTING,
        FINISHED,
    };

    BlockContext(bcos::protocol::Block::Ptr block) : m_block(std::move(block)) {}

    Status status() { return m_status; }

private:
    Status m_status;
    bcos::protocol::Block::Ptr m_block;
};
}  // namespace bcos::dispatcher