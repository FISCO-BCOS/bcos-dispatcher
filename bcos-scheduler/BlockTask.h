#pragma once

#include "interfaces/protocol/Block.h"
#include "interfaces/protocol/BlockHeader.h"
#include "interfaces/protocol/ProtocolTypeDef.h"

namespace bcos
{
namespace dispatcher
{
class BlockTask
{
public:
    bcos::protocol::Block::ConstPtr fromBlock() const;
    void setFromBlock(bcos::protocol::Block::ConstPtr block);

private:
    bcos::protocol::Block::ConstPtr m_fromBlock;
    bcos::protocol::BlockHeader::Ptr m_toBlockHeader;
};
}  // namespace dispatcher
}  // namespace bcos