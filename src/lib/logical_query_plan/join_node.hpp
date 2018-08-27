#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"

#include "abstract_lqp_node.hpp"
#include "expression/abstract_expression.hpp"
#include "lqp_cxlumn_reference.hpp"

namespace opossum {

/**
 * This node type is used to represent any type of Join, including cross products.
 */
class JoinNode : public EnableMakeForLQPNode<JoinNode>, public AbstractLQPNode {
 public:
  // Constructor for Cross Joins. join_mode has to be JoinMode::Cross
  explicit JoinNode(const JoinMode join_mode);

  // Constructor for predicated joins
  explicit JoinNode(const JoinMode join_mode, const std::shared_ptr<AbstractExpression>& join_predicate);

  std::string description() const override;
  const std::vector<std::shared_ptr<AbstractExpression>>& cxlumn_expressions() const override;
  std::vector<std::shared_ptr<AbstractExpression>> node_expressions() const override;
  std::shared_ptr<TableStatistics> derive_statistics_from(
      const std::shared_ptr<AbstractLQPNode>& left_input,
      const std::shared_ptr<AbstractLQPNode>& right_input) const override;

  const JoinMode join_mode;
  const std::shared_ptr<AbstractExpression> join_predicate;

 protected:
  std::shared_ptr<AbstractLQPNode> _on_shallow_copy(LQPNodeMapping& node_mapping) const override;
  bool _on_shallow_equals(const AbstractLQPNode& rhs, const LQPNodeMapping& node_mapping) const override;

 private:
  mutable std::vector<std::shared_ptr<AbstractExpression>> _cxlumn_expressions;
};

}  // namespace opossum
