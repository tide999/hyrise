#pragma once

#include <string>

#include "abstract_lqp_node.hpp"

namespace opossum {

/**
 * This node type represents the SHOW COLUMNS management command.
 */
class ShowCxlumnsNode : public EnableMakeForLQPNode<ShowCxlumnsNode>, public AbstractLQPNode {
 public:
  explicit ShowCxlumnsNode(const std::string& table_name);

  std::string description() const override;

  const std::string& table_name() const;

 protected:
  std::shared_ptr<AbstractLQPNode> _on_shallow_copy(LQPNodeMapping& node_mapping) const override;
  bool _on_shallow_equals(const AbstractLQPNode& rhs, const LQPNodeMapping& node_mapping) const override;

 private:
  const std::string _table_name;
};

}  // namespace opossum
