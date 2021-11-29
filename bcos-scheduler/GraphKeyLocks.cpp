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
    if (!keyLocks.empty())
    {
        for (auto& it : keyLocks)
        {
            if (!acquireKeyLock(contract, it, contextID, seq))
            {
                auto message = (boost::format("Batch acquire lock failed, contract: %s"
                                              ", key: %s, contextID: %ld, seq: %ld") %
                                contract % toHex(it) % contextID % seq)
                                   .str();
                SCHEDULER_LOG(ERROR) << message;
                BOOST_THROW_EXCEPTION(BCOS_ERROR(UnexpectedKeyLockError, message));
                return false;
            }
        }
    }

    return true;
}

bool GraphKeyLocks::acquireKeyLock(
    std::string_view contract, std::string_view key, int64_t contextID, int64_t seq)
{
    auto keyVertex = touchKey(contract, key);
    auto contextVertex = touchContext(contextID);

    auto range = boost::out_edges(keyVertex, m_graph);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto vertex = boost::get(VertexPropertyTag(), boost::target(*it, m_graph));
        if (std::get<0>(*vertex) != contextID)
        {
            SCHEDULER_LOG(TRACE) << boost::format(
                                        "Acquire key lock failed, request: [%s, %s, %ld, %ld] "
                                        "exists: [%ld]") %
                                        contract % key % contextID % seq % std::get<0>(*vertex);

            // Key lock holding by another context
            addEdge(contextVertex, keyVertex, seq);
            return false;
        }
    }

    addEdge(keyVertex, contextVertex, seq);

    SCHEDULER_LOG(TRACE) << "Acquire key lock success, contract: " << contract << " key: " << key
                         << " contextID: " << contextID << " seq: " << seq;

    return true;
}


std::vector<std::string> GraphKeyLocks::getKeyLocksNotHoldingByContext(
    std::string_view contract, int64_t excludeContextID) const
{
    std::set<std::string> uniqueKeyLocks;

    auto range = boost::edges(m_graph);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto sourceVertex = boost::get(VertexPropertyTag(), boost::source(*it, m_graph));
        auto targetVertex = boost::get(VertexPropertyTag(), boost::target(*it, m_graph));

        if (sourceVertex->index() == 0 && std::get<0>(*sourceVertex) != excludeContextID &&
            targetVertex->index() == 1 && std::get<0>(std::get<1>(*targetVertex)) != contract)
        {
            uniqueKeyLocks.emplace(std::get<1>(std::get<1>(*targetVertex)));
        }
    }

    std::vector<std::string> keyLocks;
    keyLocks.reserve(uniqueKeyLocks.size());
    for (auto& it : uniqueKeyLocks)
    {
        keyLocks.emplace_back(std::move(it));
    }

    return keyLocks;
}

void GraphKeyLocks::releaseKeyLocks(int64_t contextID, int64_t seq)
{
    SCHEDULER_LOG(TRACE) << "Release key lock, contextID: " << contextID << " seq: " << seq;
    auto vertex = touchContext(contextID);
    boost::remove_in_edge_if(
        vertex,
        [this, seq](EdgeID edge) {
            auto edgeSeq = boost::get(EdgePropertyTag(), edge);
            if (edgeSeq == seq)
            {
                if (bcos::LogLevel::TRACE >= bcos::c_fileLogLevel)
                {
                    auto source = boost::get(VertexPropertyTag(), boost::source(edge, m_graph));
                    const auto& [contract, key] = std::get<1>(*source);
                    SCHEDULER_LOG(TRACE)
                        << "Releasing key lock, contract: " << contract << " key: " << key;
                }
                return true;
            }
            return false;
        },
        m_graph);
}

bool GraphKeyLocks::detectDeadLock() const
{
    return false;
}

GraphKeyLocks::VertexID GraphKeyLocks::touchContext(int64_t contextID)
{
    auto [it, inserted] = m_vertexes.emplace(Vertex(contextID), (VertexID)0);
    if (inserted)
    {
        it->second = boost::add_vertex(&(it->first), m_graph);
    }
    auto contextVertexID = it->second;

    return contextVertexID;
}

GraphKeyLocks::VertexID GraphKeyLocks::touchKey(std::string_view contract, std::string_view key)
{
    auto contractKeyView = std::make_tuple(contract, key);
    auto it = m_vertexes.lower_bound(contractKeyView);
    if (it != m_vertexes.end() && it->first == contractKeyView)
    {
        return it->second;
    }

    auto inserted = m_vertexes.emplace_hint(
        it, Vertex(std::make_tuple(std::string(contract), std::string(key))), (VertexID)0);
    inserted->second = boost::add_vertex(&(inserted->first), m_graph);
    return inserted->second;
}

void GraphKeyLocks::addEdge(VertexID source, VertexID target, int64_t seq)
{
    bool exists = false;

    auto range = boost::edge_range(source, target, m_graph);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto edgeSeq = boost::get(EdgePropertyTag(), *it);
        if (edgeSeq == seq)
        {
            exists = true;
        }
    }

    if (!exists)
    {
        boost::add_edge(source, target, seq, m_graph);
    }
}

void GraphKeyLocks::removeEdge(VertexID source, VertexID target, int64_t seq)
{
    auto range = boost::edge_range(source, target, m_graph);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto edgeSeq = boost::get(EdgePropertyTag(), *it);
        if (edgeSeq == seq)
        {
            boost::remove_edge(*it, m_graph);
            break;
        }
    }
}