#include "gtest/gtest.h"

#include "expression/expression_functional.hpp"
#include "logical_query_plan/join_node.hpp"
#include "logical_query_plan/mock_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "logical_query_plan/projection_node.hpp"
#include "logical_query_plan/sort_node.hpp"
#include "logical_query_plan/union_node.hpp"
#include "optimizer/strategy/cxlumn_pruning_rule.hpp"

#include "strategy_base_test.hpp"
#include "testing_assert.hpp"

using namespace opossum::expression_functional;  // NOLINT

namespace opossum {

class CxlumnPruningRuleTest : public StrategyBaseTest {
 public:
  void SetUp() override {
    node_a = MockNode::make(
        MockNode::CxlumnDefinitions{{DataType::Int, "a"}, {DataType::Int, "b"}, {DataType::Int, "c"}}, "a");
    node_b = MockNode::make(
        MockNode::CxlumnDefinitions{{DataType::Int, "u"}, {DataType::Int, "v"}, {DataType::Int, "w"}}, "b");
    node_c = MockNode::make(MockNode::CxlumnDefinitions{{DataType::Int, "x"}, {DataType::Int, "y"}}, "c");

    a = node_a->get_cxlumn("a");
    b = node_a->get_cxlumn("b");
    c = node_a->get_cxlumn("c");
    u = node_b->get_cxlumn("u");
    v = node_b->get_cxlumn("v");
    w = node_b->get_cxlumn("w");
    x = node_c->get_cxlumn("x");
    y = node_c->get_cxlumn("y");

    rule = std::make_shared<CxlumnPruningRule>();
  }

  std::shared_ptr<CxlumnPruningRule> rule;
  std::shared_ptr<MockNode> node_a, node_b, node_c;
  LQPCxlumnReference a, b, c, u, v, w, x, y;
};

TEST_F(CxlumnPruningRuleTest, NoUnion) {
  // clang-format off
  const auto lqp =
  ProjectionNode::make(expression_vector(add_(mul_(a, u), 5)),
    PredicateNode::make(greater_than_(5, c),
      JoinNode::make(JoinMode::Inner, greater_than_(v, a),
        node_a,
        SortNode::make(expression_vector(w), std::vector<OrderByMode>{OrderByMode::Ascending},  // NOLINT
          node_b))));
  // clang-format on

  // clang-format off
  const auto expected_lqp =
  ProjectionNode::make(expression_vector(add_(mul_(a, u), 5)),
    PredicateNode::make(greater_than_(5, c),
      JoinNode::make(JoinMode::Inner, greater_than_(v, a),
        ProjectionNode::make(expression_vector(a, c),
          node_a),
        SortNode::make(expression_vector(w), std::vector<OrderByMode>{OrderByMode::Ascending},  // NOLINT
          node_b))));
  // clang-format on

  const auto actual_lqp = apply_rule(rule, lqp);

  EXPECT_LQP_EQ(actual_lqp, expected_lqp);
}

TEST_F(CxlumnPruningRuleTest, WithUnion) {
  // clang-format off
  const auto lqp =
  ProjectionNode::make(expression_vector(a),
    UnionNode::make(UnionMode::Positions,
      PredicateNode::make(greater_than_(a, 5), node_a),
      PredicateNode::make(greater_than_(b, 5), node_a)));
  // clang-format on

  // clang-format off
  const auto expected_lqp =
  ProjectionNode::make(expression_vector(a),
    UnionNode::make(UnionMode::Positions,
      PredicateNode::make(greater_than_(a, 5),
        ProjectionNode::make(expression_vector(a, b),
          node_a)),
        PredicateNode::make(greater_than_(b, 5),
          ProjectionNode::make(expression_vector(a, b),
            node_a))));
  // clang-format on

  const auto actual_lqp = apply_rule(rule, lqp);

  EXPECT_LQP_EQ(actual_lqp, expected_lqp);
}

TEST_F(CxlumnPruningRuleTest, WithMultipleProjections) {
  // clang-format off
  const auto lqp =
  ProjectionNode::make(expression_vector(a),
    PredicateNode::make(greater_than_(mul_(a, b), 5),
      ProjectionNode::make(expression_vector(a, b, mul_(a, b), c),
        PredicateNode::make(greater_than_(mul_(a, 2), 5),
          ProjectionNode::make(expression_vector(a, b, mul_(a, 2), c),
            node_a)))));
  // clang-format on

  // clang-format off
  const auto expected_lqp =
  ProjectionNode::make(expression_vector(a),
    PredicateNode::make(greater_than_(mul_(a, b), 5),
      ProjectionNode::make(expression_vector(a, b, mul_(a, b)),
        PredicateNode::make(greater_than_(mul_(a, 2), 5),
          ProjectionNode::make(expression_vector(a, b, mul_(a, 2)),
            ProjectionNode::make(expression_vector(a, b),
              node_a))))));
  // clang-format on

  const auto actual_lqp = apply_rule(rule, lqp);

  EXPECT_LQP_EQ(actual_lqp, expected_lqp);
}

}  // namespace opossum
