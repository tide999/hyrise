#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../../base_test.hpp"
#include "gtest/gtest.h"

#include "expression/abstract_expression.hpp"
#include "expression/expression_functional.hpp"
#include "logical_query_plan/mock_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "logical_query_plan/stored_table_node.hpp"
#include "optimizer/strategy/index_scan_rule.hpp"
#include "optimizer/strategy/strategy_base_test.hpp"
#include "statistics/cxlumn_statistics.hpp"
#include "statistics/table_statistics.hpp"
#include "storage/chunk_encoder.hpp"
#include "storage/dictionary_segment.hpp"
#include "storage/index/adaptive_radix_tree/adaptive_radix_tree_index.hpp"
#include "storage/index/group_key/composite_group_key_index.hpp"
#include "storage/index/group_key/group_key_index.hpp"
#include "storage/storage_manager.hpp"
#include "utils/assert.hpp"

using namespace opossum::expression_functional;  // NOLINT

namespace opossum {

class IndexScanRuleTest : public StrategyBaseTest {
 public:
  void SetUp() override {
    table = load_table("src/test/tables/int_int_int.tbl", Chunk::MAX_SIZE);
    StorageManager::get().add_table("a", table);
    ChunkEncoder::encode_all_chunks(StorageManager::get().get_table("a"));

    rule = std::make_shared<IndexScanRule>();

    stored_table_node = StoredTableNode::make("a");
    a = stored_table_node->get_cxlumn("a");
    b = stored_table_node->get_cxlumn("b");
    c = stored_table_node->get_cxlumn("c");
  }

  std::shared_ptr<TableStatistics> generate_mock_statistics(float row_count = 0.0f) {
    std::vector<std::shared_ptr<const BaseCxlumnStatistics>> cxlumn_statistics;
    cxlumn_statistics.emplace_back(std::make_shared<CxlumnStatistics<int32_t>>(0.0f, 10, 0, 20));
    cxlumn_statistics.emplace_back(std::make_shared<CxlumnStatistics<int32_t>>(0.0f, 10, 0, 20));
    cxlumn_statistics.emplace_back(std::make_shared<CxlumnStatistics<int32_t>>(0.0f, 10, 0, 20'000));
    return std::make_shared<TableStatistics>(TableStatistics{TableType::Data, row_count, cxlumn_statistics});
  }

  std::shared_ptr<IndexScanRule> rule;
  std::shared_ptr<StoredTableNode> stored_table_node;
  std::shared_ptr<Table> table;
  LQPCxlumnReference a, b, c;
};

TEST_F(IndexScanRuleTest, NoIndexScanWithoutIndex) {
  auto statistics_mock = generate_mock_statistics();
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(a, 10));
  predicate_node_0->set_left_input(stored_table_node);

  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_0);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
}

TEST_F(IndexScanRuleTest, NoIndexScanWithIndexOnOtherColumn) {
  table->create_index<GroupKeyIndex>({CxlumnID{2}});

  auto statistics_mock = generate_mock_statistics();
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(a, 10));
  predicate_node_0->set_left_input(stored_table_node);

  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_0);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
}

TEST_F(IndexScanRuleTest, NoIndexScanWithMultiColumnIndex) {
  table->create_index<CompositeGroupKeyIndex>({CxlumnID{2}, CxlumnID{1}});

  auto statistics_mock = generate_mock_statistics();
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(c, 10));
  predicate_node_0->set_left_input(stored_table_node);

  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_0);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
}

TEST_F(IndexScanRuleTest, NoIndexScanWithTwoColumnPredicate) {
  auto statistics_mock = generate_mock_statistics();
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(c, b));
  predicate_node_0->set_left_input(stored_table_node);

  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_0);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
}

TEST_F(IndexScanRuleTest, NoIndexScanWithHighSelectivity) {
  table->create_index<GroupKeyIndex>({CxlumnID{2}});

  auto statistics_mock = generate_mock_statistics(80'000);
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(c, 10));
  predicate_node_0->set_left_input(stored_table_node);

  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_0);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
}

TEST_F(IndexScanRuleTest, NoIndexScanIfNotGroupKey) {
  table->create_index<AdaptiveRadixTreeIndex>({CxlumnID{2}});

  auto statistics_mock = generate_mock_statistics(1'000'000);
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(c, 10));
  predicate_node_0->set_left_input(stored_table_node);

  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_0);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
}

TEST_F(IndexScanRuleTest, IndexScanWithIndex) {
  table->create_index<GroupKeyIndex>({CxlumnID{2}});

  auto statistics_mock = generate_mock_statistics(1'000'000);
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(c, 19'900));
  predicate_node_0->set_left_input(stored_table_node);

  EXPECT_EQ(predicate_node_0->scan_type, ScanType::TableScan);
  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_0);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::IndexScan);
}

TEST_F(IndexScanRuleTest, IndexScanOnlyOnOutputOfStoredTableNode) {
  table->create_index<GroupKeyIndex>({CxlumnID{2}});

  auto statistics_mock = generate_mock_statistics(1'000'000);
  table->set_table_statistics(statistics_mock);

  auto predicate_node_0 = PredicateNode::make(greater_than_(c, 19'900));
  predicate_node_0->set_left_input(stored_table_node);

  auto predicate_node_1 = PredicateNode::make(less_than_(b, 15));
  predicate_node_1->set_left_input(predicate_node_0);

  auto reordered = StrategyBaseTest::apply_rule(rule, predicate_node_1);
  EXPECT_EQ(predicate_node_0->scan_type, ScanType::IndexScan);
  EXPECT_EQ(predicate_node_1->scan_type, ScanType::TableScan);
}

}  // namespace opossum
