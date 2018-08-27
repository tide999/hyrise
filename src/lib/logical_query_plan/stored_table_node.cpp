#include "stored_table_node.hpp"

#include "expression/lqp_cxlumn_expression.hpp"
#include "storage/storage_manager.hpp"
#include "storage/table.hpp"

namespace opossum {

StoredTableNode::StoredTableNode(const std::string& table_name)
    : AbstractLQPNode(LQPNodeType::StoredTable), table_name(table_name) {}

LQPCxlumnReference StoredTableNode::get_cxlumn(const std::string& name) const {
  const auto table = StorageManager::get().get_table(table_name);
  const auto cxlumn_id = table->cxlumn_id_by_name(name);
  return {shared_from_this(), cxlumn_id};
}

void StoredTableNode::set_excluded_chunk_ids(const std::vector<ChunkID>& chunks) { _excluded_chunk_ids = chunks; }

const std::vector<ChunkID>& StoredTableNode::excluded_chunk_ids() const { return _excluded_chunk_ids; }

std::string StoredTableNode::description() const { return "[StoredTable] Name: '" + table_name + "'"; }

const std::vector<std::shared_ptr<AbstractExpression>>& StoredTableNode::cxlumn_expressions() const {
  // Need to initialize the expressions lazily because they will have a weak_ptr to this node and we can't obtain that
  // in the constructor
  if (!_expressions) {
    const auto table = StorageManager::get().get_table(table_name);

    _expressions.emplace(table->cxlumn_count());
    for (auto cxlumn_id = CxlumnID{0}; cxlumn_id < table->cxlumn_count(); ++cxlumn_id) {
      (*_expressions)[cxlumn_id] =
          std::make_shared<LQPCxlumnExpression>(LQPCxlumnReference{shared_from_this(), cxlumn_id});
    }
  }

  return *_expressions;
}

std::shared_ptr<TableStatistics> StoredTableNode::derive_statistics_from(
    const std::shared_ptr<AbstractLQPNode>& left_input, const std::shared_ptr<AbstractLQPNode>& right_input) const {
  DebugAssert(!left_input && !right_input, "StoredTableNode must be leaf");
  return StorageManager::get().get_table(table_name)->table_statistics();
}

std::shared_ptr<AbstractLQPNode> StoredTableNode::_on_shallow_copy(LQPNodeMapping& node_mapping) const {
  const auto copy = make(table_name);
  copy->set_excluded_chunk_ids(_excluded_chunk_ids);
  return copy;
}

bool StoredTableNode::_on_shallow_equals(const AbstractLQPNode& rhs, const LQPNodeMapping& node_mapping) const {
  const auto& stored_table_node = static_cast<const StoredTableNode&>(rhs);
  return table_name == stored_table_node.table_name && _excluded_chunk_ids == stored_table_node._excluded_chunk_ids;
}

}  // namespace opossum
