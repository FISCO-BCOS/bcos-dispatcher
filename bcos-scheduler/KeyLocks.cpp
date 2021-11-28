#include "KeyLocks.h"
#include "Common.h"
#include <assert.h>
#include <bcos-framework/libutilities/DataConvertUtility.h>
#include <bcos-framework/libutilities/Error.h>
#include <boost/core/ignore_unused.hpp>
#include <boost/format.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/edge_list.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/throw_exception.hpp>

using namespace bcos::scheduler;

bool KeyLocks::batchAcquireKeyLock(std::string_view contract, gsl::span<std::string const> keyLocks,
    int64_t contextID, int64_t seq)
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

bool KeyLocks::acquireKeyLock(
    std::string_view contract, std::string_view key, int64_t contextID, int64_t seq)
{
    auto it = m_keyLocks.get<1>().find(std::tuple{contract, key, true});
    if (it != m_keyLocks.get<1>().end() && it->contextID != contextID)
    {
        // Key lock holding by other context
        SCHEDULER_LOG(TRACE) << boost::format(
                                    "Acquire key lock failed, request: [%s, %s, %ld, %ld] "
                                    "exists: [%ld, %ld]") %
                                    contract % key % contextID % seq % it->contextID % it->seq;

        // Another context is owing the key
        return false;
    }

    // No other context holding the key lock, accquire it
    it = m_keyLocks.get<1>().find(std::tuple{contract, key, false});
    if (it != m_keyLocks.get<1>().end())
    {
        m_keyLocks.get<1>().modify(it, [](KeyLockItem& item) { item.holding = true; });
    }
    else
    {
        m_keyLocks.emplace(
            KeyLockItem{std::string(contract), std::string(key), contextID, seq, true});
    }

    SCHEDULER_LOG(TRACE) << "Acquire key lock success, contract: " << contract << " key: " << key
                         << " contextID: " << contextID << " seq: " << seq;


    return true;
}


std::vector<std::string> KeyLocks::getContractKeyLocksNotHoldingByContext(
    std::string_view contract, int64_t excludeContextID) const
{
    std::vector<std::string> results;
    auto count = m_keyLocks.get<2>().count(std::make_tuple(contract, true));

    if (count > 0)
    {
        auto range = m_keyLocks.get<2>().equal_range(std::make_tuple(contract, true));

        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->contextID != excludeContextID)
            {
                results.emplace_back(it->key);
            }
        }
    }

    return results;
}

void KeyLocks::releaseKeyLocks(int64_t contextID, int64_t seq)
{
    SCHEDULER_LOG(TRACE) << "Release key lock, contextID: " << contextID << " seq: " << seq;

    for (auto holding : {true, false})
    {
        auto range = m_keyLocks.get<3>().equal_range(std::tuple{contextID, seq, holding});

        if (bcos::LogLevel::TRACE >= bcos::c_fileLogLevel)
        {
            for (auto it = range.first; it != range.second; ++it)
            {
                SCHEDULER_LOG(TRACE) << "Releasing key lock, contract: " << it->contract
                                     << " key: " << toHex(it->key)
                                     << " contextID: " << it->contextID << " seq: " << it->seq;
            }
        }

        m_keyLocks.get<3>().erase(range.first, range.second);
    }
}

// Find cycle using DFS
class GraphVisitor : public boost::default_dfs_visitor
{
public:
    template <typename Vertex, typename Graph>
    void discover_vertex(Vertex, Graph) const
    {}

    template <typename Edge, typename Graph>
    void back_edge(Edge, Graph) const
    {
        // items.emplace_front(edge);
        // ++count;
    }

    template <typename Edge, typename Graph>
    void examine_edge(Edge, Graph) const
    {}

    std::forward_list<size_t> items;
    size_t count = 0;
};

auto KeyLocks::detectDeadLock() const
{
    using Graph = boost::adjacency_list<>;
    using Vertex =
        std::variant<std::tuple<int64_t, int64_t>, std::tuple<std::string_view, std::string_view>>;
    using VertexID = Graph::vertex_descriptor;

    Graph graph;
    std::map<Vertex, VertexID> vertex2ID;

    // Add context vertex
    for (auto& it : m_keyLocks.get<0>())
    {
        auto [contextIt, contextInserted] =
            vertex2ID.emplace(std::make_tuple(it.contextID, it.seq), 0);
        if (contextInserted)
        {
            contextIt->second = boost::add_vertex(graph);
        }

        auto [keyIt, keyInserted] = vertex2ID.emplace(std::make_tuple(it.contract, it.key), 0);
        if (keyInserted)
        {
            keyIt->second = boost::add_vertex(graph);
        }

        if (it.holding)
        {
            boost::add_edge(keyIt->second, contextIt->second, graph);
        }
        else
        {
            boost::add_edge(contextIt->second, keyIt->second, graph);
        }
    }


    return false;
}