#pragma once

#include <tbb/concurrent_unordered_map.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/edge_list.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/graph/visitors.hpp>
#include <any>
#include <forward_list>
#include <functional>
#include <gsl/span>
#include <set>
#include <string_view>
#include <variant>

namespace bcos::scheduler
{
class GraphKeyLocks
{
public:
    using Ptr = std::shared_ptr<GraphKeyLocks>;

    GraphKeyLocks() = default;
    GraphKeyLocks(const GraphKeyLocks&) = delete;
    GraphKeyLocks(GraphKeyLocks&&) = delete;
    GraphKeyLocks& operator=(const GraphKeyLocks&) = delete;
    GraphKeyLocks& operator=(GraphKeyLocks&&) = delete;

    bool batchAcquireKeyLock(std::string_view contract, gsl::span<std::string const> keyLocks,
        int64_t contextID, int64_t seq);

    bool acquireKeyLock(
        std::string_view contract, std::string_view key, int64_t contextID, int64_t seq);

    std::vector<std::string> getContractKeyLocksNotHoldingByContext(
        std::string_view contract, int64_t excludeContextID) const;

    void releaseKeyLocks(int64_t contextID, int64_t seq);

    auto detectDeadLock() const;

private:
    using Graph = boost::adjacency_list<>;
    using Vertex =
        std::variant<std::tuple<int64_t, int64_t>, int64_t, std::tuple<std::string, std::string>>;
    using VertexID = Graph::vertex_descriptor;

    Graph m_graph;
    std::map<Vertex, VertexID, std::less<>> m_vertexes;

    std::tuple<GraphKeyLocks::VertexID, GraphKeyLocks::VertexID> touchContext(
        int64_t contextID, int64_t seq);
    VertexID touchKey(std::string_view contract, std::string_view key);
};

}  // namespace bcos::scheduler