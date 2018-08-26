#pragma once

#include <limits>
#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "join_graph_edge.hpp"
#include "logical_query_plan/abstract_lqp_node.hpp"
#include "logical_query_plan/join_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "types.hpp"

namespace opossum {

/**
 * TODO(moritz) doc
 */
class JoinGraph final {
 public:
  /**
   * Tries to turn the subplan rooted at @lqp into a JoinGraph.
   * @return nullptr, it this is impossible (e.g., lqp is a Sort/AggregateNode)
   */
  static std::optional<JoinGraph> from_lqp(const std::shared_ptr<AbstractLQPNode>& lqp);

  JoinGraph() = default;
  JoinGraph(const std::vector<std::shared_ptr<AbstractLQPNode>>& vertices,
            const std::vector<JoinGraphEdge>& edges);

  /**
   * Find all predicates that use exactly the nodes in vertex set
   */
  std::vector<std::shared_ptr<AbstractExpression>> find_local_predicates(const size_t vertex_idx) const;

  /**
   * Find all predicates that "connect" the two vertex sets, i.e. have operands in both of them
   */
  std::vector<std::shared_ptr<AbstractExpression>> find_join_predicates(
  const JoinGraphVertexSet& vertex_set_a, const JoinGraphVertexSet& vertex_set_b) const;

  void print(std::ostream& stream = std::cout) const;

  const std::vector<std::shared_ptr<AbstractLQPNode>> vertices;
  std::vector<JoinGraphEdge> edges;
};

}  // namespace opossum
