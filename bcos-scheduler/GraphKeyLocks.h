#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/edge_list.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/graph/properties.hpp>
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

    std::vector<std::string> getKeyLocksNotHoldingByContext(
        std::string_view contract, int64_t excludeContextID) const;

    void releaseKeyLocks(int64_t contextID, int64_t seq);

    bool detectDeadLock() const;

    struct Vertex : public std::variant<int64_t, std::tuple<std::string, std::string>>
    {
        using std::variant<int64_t, std::tuple<std::string, std::string>>::variant;

        bool operator==(const std::tuple<std::string_view, std::string_view>& rhs) const
        {
            if (index() != 1)
            {
                return false;
            }

            auto view = std::make_tuple(std::string_view(std::get<0>(std::get<1>(*this))),
                std::string_view(std::get<1>(std::get<1>(*this))));
            return view == rhs;
        }
    };

private:
    struct VertexIterator
    {
        using kind = boost::vertex_property_tag;
    };
    using VertexProperty = boost::property<VertexIterator, const Vertex*>;
    struct EdgeSeq
    {
        using kind = boost::edge_property_tag;
    };
    using EdgeProperty = boost::property<EdgeSeq, int64_t>;

    using Graph = boost::adjacency_list<boost::multisetS, boost::multisetS, boost::bidirectionalS,
        VertexProperty, EdgeProperty>;
    using VertexPropertyTag = boost::property_map<Graph, VertexIterator>::const_type;
    using EdgePropertyTag = boost::property_map<Graph, EdgeSeq>::const_type;
    using VertexID = Graph::vertex_descriptor;
    using EdgeID = Graph::edge_descriptor;

    Graph m_graph;
    std::map<Vertex, VertexID, std::less<>> m_vertexes;

    VertexID touchContext(int64_t contextID);
    VertexID touchKey(std::string_view contract, std::string_view key);

    void addEdge(VertexID source, VertexID target, int64_t seq);
    void removeEdge(VertexID source, VertexID target, int64_t seq);
};

}  // namespace bcos::scheduler

namespace std
{
inline bool operator<(const bcos::scheduler::GraphKeyLocks::Vertex& lhs,
    const std::tuple<std::string_view, std::string_view>& rhs)
{
    if (lhs.index() != 1)
    {
        return false;
    }

    auto view = std::make_tuple(std::string_view(std::get<0>(std::get<1>(lhs))),
        std::string_view(std::get<1>(std::get<1>(lhs))));
    return view < rhs;
}

inline bool operator<(const std::tuple<std::string_view, std::string_view>& lhs,
    const bcos::scheduler::GraphKeyLocks::Vertex& rhs)
{
    if (rhs.index() != 1)
    {
        return true;
    }

    auto view = std::make_tuple(std::string_view(std::get<0>(std::get<1>(rhs))),
        std::string_view(std::get<1>(std::get<1>(rhs))));
    return lhs < view;
}
}  // namespace std