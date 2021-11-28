#include "GraphKeyLocks.h"
#include "Common.h"
#include <bcos-framework/libutilities/DataConvertUtility.h>
#include <bcos-framework/libutilities/Error.h>
#include <boost/core/ignore_unused.hpp>
#include <boost/format.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/detail/adjacency_list.hpp>
#include <boost/graph/edge_list.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/throw_exception.hpp>

using namespace bcos::scheduler;

bool GraphKeyLocks::batchAcquireKeyLock(std::string_view contract,
    gsl::span<std::string const> keyLocks, int64_t contextID, int64_t seq)
{
    boost::ignore_unused(contract, keyLocks, contextID, seq);

    return true;
}

bool GraphKeyLocks::acquireKeyLock(
    std::string_view contract, std::string_view key, int64_t contextID, int64_t seq)
{
    auto keyVertex = touchKey(contract, key);
    auto [contextVertex, contextSeqVertex] = touchContext(contextID, seq);

    auto [waitEdge, exists] = boost::edge(contextVertex, keyVertex, m_graph);
    if (exists)
    {
        // Already waiting
        return false;
    }

    auto range = boost::out_edges(keyVertex, m_graph);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto contextVertex = it->m_target;
    }

    return true;
}


std::vector<std::string> GraphKeyLocks::getContractKeyLocksNotHoldingByContext(
    std::string_view contract, int64_t excludeContextID) const
{
    boost::ignore_unused(contract, excludeContextID);
    return {};
}

void GraphKeyLocks::releaseKeyLocks(int64_t contextID, int64_t seq)
{
    boost::ignore_unused(contextID, seq);
}

auto GraphKeyLocks::detectDeadLock() const
{
    return false;
}

std::tuple<GraphKeyLocks::VertexID, GraphKeyLocks::VertexID> GraphKeyLocks::touchContext(
    int64_t contextID, int64_t seq)
{
    Vertex contextVertex(contextID);
    auto [it, inserted] = m_vertexes.emplace(contextVertex, 0);
    if (inserted)
    {
        it->second = boost::add_vertex(m_graph);
    }
    auto contextVertexID = it->second;

    Vertex contextSeqVertex(std::make_tuple(contextID, seq));
    std::tie(it, inserted) = m_vertexes.emplace(contextSeqVertex, 0);
    if (inserted)
    {
        it->second = boost::add_vertex(m_graph);
    }
    auto contextSeqVertexID = it->second;

    return {contextVertexID, contextSeqVertexID};
}

GraphKeyLocks::VertexID GraphKeyLocks::touchKey(std::string_view contract, std::string_view key)
{
    auto contractKeyView = std::make_tuple(contract, key);
    auto it = m_vertexes.lower_bound(contractKeyView);
    if (it != m_vertexes.end() && it->first == contractKeyView)
    {
        return it->second;
    }

    auto inserted =
        m_vertexes.emplace_hint(it, Vertex(std::string(contract), std::string(key)), m_count++);
    return inserted->second;
}