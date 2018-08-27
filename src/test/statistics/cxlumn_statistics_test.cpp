#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "all_parameter_variant.hpp"
#include "base_test.hpp"
#include "gtest/gtest.h"
#include "operators/table_scan.hpp"
#include "operators/table_wrapper.hpp"
#include "statistics/cxlumn_statistics.hpp"
#include "statistics/generate_table_statistics.hpp"

namespace opossum {

class CxlumnStatisticsTest : public BaseTest {
 protected:
  void SetUp() override {
    _table_with_different_cxlumn_types = load_table("src/test/tables/int_float_double_string.tbl", Chunk::MAX_SIZE);
    auto table_statistics1 = generate_table_statistics(*_table_with_different_cxlumn_types);
    _cxlumn_statistics_int = std::dynamic_pointer_cast<CxlumnStatistics<int32_t>>(
        std::const_pointer_cast<BaseCxlumnStatistics>(table_statistics1.cxlumn_statistics()[0]));
    _cxlumn_statistics_float = std::dynamic_pointer_cast<CxlumnStatistics<float>>(
        std::const_pointer_cast<BaseCxlumnStatistics>(table_statistics1.cxlumn_statistics()[1]));
    _cxlumn_statistics_double = std::dynamic_pointer_cast<CxlumnStatistics<double>>(
        std::const_pointer_cast<BaseCxlumnStatistics>(table_statistics1.cxlumn_statistics()[2]));
    _cxlumn_statistics_string = std::dynamic_pointer_cast<CxlumnStatistics<std::string>>(
        std::const_pointer_cast<BaseCxlumnStatistics>(table_statistics1.cxlumn_statistics()[3]));

    _table_uniform_distribution = load_table("src/test/tables/int_equal_distribution.tbl", Chunk::MAX_SIZE);
    auto table_statistics2 = generate_table_statistics(*_table_uniform_distribution);
    _cxlumn_statistics_uniform_cxlumns = table_statistics2.cxlumn_statistics();
  }

  // For single value scans (i.e. all but BETWEEN)
  template <typename T>
  void predict_selectivities_and_compare(const std::shared_ptr<CxlumnStatistics<T>>& cxlumn_statistic,
                                         const PredicateCondition predicate_condition, const std::vector<T>& values,
                                         const std::vector<float>& expected_selectivities) {
    auto expected_selectivities_itr = expected_selectivities.begin();
    for (const auto& value : values) {
      auto result_container =
          cxlumn_statistic->estimate_predicate_with_value(predicate_condition, AllTypeVariant(value));
      EXPECT_FLOAT_EQ(result_container.selectivity, *expected_selectivities_itr++);
    }
  }

  // For two cxlumn scans (type of value1 is CxlumnID)
  void predict_selectivities_and_compare(
      const std::shared_ptr<Table>& table,
      const std::vector<std::shared_ptr<const BaseCxlumnStatistics>>& cxlumn_statistics,
      const PredicateCondition predicate_condition) {
    auto table_wrapper = std::make_shared<TableWrapper>(table);
    table_wrapper->execute();
    auto row_count = table->row_count();
    for (CxlumnID::base_type cxlumn_1 = 0; cxlumn_1 < cxlumn_statistics.size(); ++cxlumn_1) {
      for (CxlumnID::base_type cxlumn_2 = 0; cxlumn_2 < cxlumn_statistics.size() && cxlumn_1 != cxlumn_2; ++cxlumn_2) {
        auto result_container = cxlumn_statistics[cxlumn_1]->estimate_predicate_with_cxlumn(
            predicate_condition, *cxlumn_statistics[cxlumn_2]);
        auto table_scan =
            std::make_shared<TableScan>(table_wrapper, CxlumnID{cxlumn_1}, predicate_condition, CxlumnID{cxlumn_2});
        table_scan->execute();
        auto result_row_count = table_scan->get_output()->row_count();
        EXPECT_FLOAT_EQ(result_container.selectivity,
                        static_cast<float>(result_row_count) / static_cast<float>(row_count));
      }
    }
  }

  // For BETWEEN
  template <typename T>
  void predict_selectivities_and_compare(const std::shared_ptr<CxlumnStatistics<T>>& cxlumn_statistic,
                                         const PredicateCondition predicate_condition,
                                         const std::vector<std::pair<T, T>>& values,
                                         const std::vector<float>& expected_selectivities) {
    auto expected_selectivities_itr = expected_selectivities.begin();
    for (const auto& value_pair : values) {
      auto result_container = cxlumn_statistic->estimate_predicate_with_value(
          predicate_condition, AllTypeVariant(value_pair.first), AllTypeVariant(value_pair.second));
      EXPECT_FLOAT_EQ(result_container.selectivity, *expected_selectivities_itr++);
    }
  }

  template <typename T>
  void predict_selectivities_for_stored_procedures_and_compare(
      const std::shared_ptr<CxlumnStatistics<T>>& cxlumn_statistic, const PredicateCondition predicate_condition,
      const std::vector<T>& values2, const std::vector<float>& expected_selectivities) {
    auto expected_selectivities_itr = expected_selectivities.begin();
    for (const auto& value2 : values2) {
      auto result_container =
          cxlumn_statistic->estimate_predicate_with_value_placeholder(predicate_condition, AllTypeVariant(value2));
      EXPECT_FLOAT_EQ(result_container.selectivity, *expected_selectivities_itr++);
    }
  }

  std::shared_ptr<Table> _table_with_different_cxlumn_types;
  std::shared_ptr<CxlumnStatistics<int32_t>> _cxlumn_statistics_int;
  std::shared_ptr<CxlumnStatistics<float>> _cxlumn_statistics_float;
  std::shared_ptr<CxlumnStatistics<double>> _cxlumn_statistics_double;
  std::shared_ptr<CxlumnStatistics<std::string>> _cxlumn_statistics_string;
  std::shared_ptr<Table> _table_uniform_distribution;
  std::vector<std::shared_ptr<const BaseCxlumnStatistics>> _cxlumn_statistics_uniform_cxlumns;

  //  {below min, min, middle, max, above max}
  std::vector<int32_t> _int_values{0, 1, 3, 6, 7};
  std::vector<float> _float_values{0.f, 1.f, 3.f, 6.f, 7.f};
  std::vector<double> _double_values{0., 1., 3., 6., 7.};
  std::vector<std::string> _string_values{"a", "b", "c", "g", "h"};
};

TEST_F(CxlumnStatisticsTest, NotEqualTest) {
  PredicateCondition predicate_condition = PredicateCondition::NotEquals;

  std::vector<float> selectivities{1.f, 5.f / 6.f, 5.f / 6.f, 5.f / 6.f, 1.f};
  predict_selectivities_and_compare(_cxlumn_statistics_int, predicate_condition, _int_values, selectivities);
  predict_selectivities_and_compare(_cxlumn_statistics_float, predicate_condition, _float_values, selectivities);
  predict_selectivities_and_compare(_cxlumn_statistics_double, predicate_condition, _double_values, selectivities);
  predict_selectivities_and_compare(_cxlumn_statistics_string, predicate_condition, _string_values, selectivities);
}

TEST_F(CxlumnStatisticsTest, EqualsTest) {
  PredicateCondition predicate_condition = PredicateCondition::Equals;

  std::vector<float> selectivities{0.f, 1.f / 6.f, 1.f / 6.f, 1.f / 6.f, 0.f};
  predict_selectivities_and_compare(_cxlumn_statistics_int, predicate_condition, _int_values, selectivities);
  predict_selectivities_and_compare(_cxlumn_statistics_float, predicate_condition, _float_values, selectivities);
  predict_selectivities_and_compare(_cxlumn_statistics_double, predicate_condition, _double_values, selectivities);
  predict_selectivities_and_compare(_cxlumn_statistics_string, predicate_condition, _string_values, selectivities);
}

TEST_F(CxlumnStatisticsTest, LessThanTest) {
  PredicateCondition predicate_condition = PredicateCondition::LessThan;

  std::vector<float> selectivities_int{0.f, 0.f, 1.f / 3.f, 5.f / 6.f, 1.f};
  predict_selectivities_and_compare(_cxlumn_statistics_int, predicate_condition, _int_values, selectivities_int);

  std::vector<float> selectivities_float{0.f, 0.f, 0.4f, 1.f, 1.f};
  predict_selectivities_and_compare(_cxlumn_statistics_float, predicate_condition, _float_values, selectivities_float);
  predict_selectivities_and_compare(_cxlumn_statistics_double, predicate_condition, _double_values,
                                    selectivities_float);
}

TEST_F(CxlumnStatisticsTest, LessEqualThanTest) {
  PredicateCondition predicate_condition = PredicateCondition::LessThanEquals;

  std::vector<float> selectivities_int{0.f, 1.f / 6.f, 1.f / 2.f, 1.f, 1.f};
  predict_selectivities_and_compare(_cxlumn_statistics_int, predicate_condition, _int_values, selectivities_int);

  std::vector<float> selectivities_float{0.f, 0.f, 0.4f, 1.f, 1.f};
  predict_selectivities_and_compare(_cxlumn_statistics_float, predicate_condition, _float_values, selectivities_float);
  predict_selectivities_and_compare(_cxlumn_statistics_double, predicate_condition, _double_values,
                                    selectivities_float);
}

TEST_F(CxlumnStatisticsTest, GreaterThanTest) {
  PredicateCondition predicate_condition = PredicateCondition::GreaterThan;

  std::vector<float> selectivities_int{1.f, 5.f / 6.f, 1.f / 2.f, 0.f, 0.f};
  predict_selectivities_and_compare(_cxlumn_statistics_int, predicate_condition, _int_values, selectivities_int);

  std::vector<float> selectivities_float{1.f, 1.f, 0.6f, 0.f, 0.f};
  predict_selectivities_and_compare(_cxlumn_statistics_float, predicate_condition, _float_values, selectivities_float);
  predict_selectivities_and_compare(_cxlumn_statistics_double, predicate_condition, _double_values,
                                    selectivities_float);
}

TEST_F(CxlumnStatisticsTest, GreaterEqualThanTest) {
  PredicateCondition predicate_condition = PredicateCondition::GreaterThanEquals;

  std::vector<float> selectivities_int{1.f, 1.f, 2.f / 3.f, 1.f / 6.f, 0.f};
  predict_selectivities_and_compare(_cxlumn_statistics_int, predicate_condition, _int_values, selectivities_int);

  std::vector<float> selectivities_float{1.f, 1.f, 0.6f, 0.f, 0.f};
  predict_selectivities_and_compare(_cxlumn_statistics_float, predicate_condition, _float_values, selectivities_float);
  predict_selectivities_and_compare(_cxlumn_statistics_double, predicate_condition, _double_values,
                                    selectivities_float);
}

TEST_F(CxlumnStatisticsTest, BetweenTest) {
  PredicateCondition predicate_condition = PredicateCondition::Between;

  std::vector<std::pair<int32_t, int32_t>> int_values{{-1, 0}, {-1, 2}, {1, 2}, {0, 7}, {5, 6}, {5, 8}, {7, 8}};
  std::vector<float> selectivities_int{0.f, 1.f / 3.f, 1.f / 3.f, 1.f, 1.f / 3.f, 1.f / 3.f, 0.f};
  predict_selectivities_and_compare(_cxlumn_statistics_int, predicate_condition, int_values, selectivities_int);

  std::vector<std::pair<float, float>> float_values{{-1.f, 0.f}, {-1.f, 2.f}, {1.f, 2.f}, {0.f, 7.f},
                                                    {5.f, 6.f},  {5.f, 8.f},  {7.f, 8.f}};
  std::vector<float> selectivities_float{0.f, 1.f / 5.f, 1.f / 5.f, 1.f, 1.f / 5.f, 1.f / 5.f, 0.f};
  predict_selectivities_and_compare(_cxlumn_statistics_float, predicate_condition, float_values, selectivities_float);

  std::vector<std::pair<double, double>> double_values{{-1., 0.}, {-1., 2.}, {1., 2.}, {0., 7.},
                                                       {5., 6.},  {5., 8.},  {7., 8.}};
  predict_selectivities_and_compare(_cxlumn_statistics_double, predicate_condition, double_values, selectivities_float);
}

TEST_F(CxlumnStatisticsTest, StoredProcedureNotEqualsTest) {
  PredicateCondition predicate_condition = PredicateCondition::NotEquals;

  auto result_container_int = _cxlumn_statistics_int->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_int.selectivity, 5.f / 6.f);

  auto result_container_float =
      _cxlumn_statistics_float->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_float.selectivity, 5.f / 6.f);

  auto result_container_double =
      _cxlumn_statistics_double->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_double.selectivity, 5.f / 6.f);

  auto result_container_string =
      _cxlumn_statistics_string->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_string.selectivity, 5.f / 6.f);
}

TEST_F(CxlumnStatisticsTest, StoredProcedureEqualsTest) {
  PredicateCondition predicate_condition = PredicateCondition::Equals;

  auto result_container_int = _cxlumn_statistics_int->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_int.selectivity, 1.f / 6.f);

  auto result_container_float =
      _cxlumn_statistics_float->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_float.selectivity, 1.f / 6.f);

  auto result_container_double =
      _cxlumn_statistics_double->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_double.selectivity, 1.f / 6.f);

  auto result_container_string =
      _cxlumn_statistics_string->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_string.selectivity, 1.f / 6.f);
}

TEST_F(CxlumnStatisticsTest, StoredProcedureOpenEndedTest) {
  // OpLessThan, OpGreaterThan, OpLessThanEquals, OpGreaterThanEquals are same for stored procedures
  PredicateCondition predicate_condition = PredicateCondition::LessThan;

  auto result_container_int = _cxlumn_statistics_int->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_int.selectivity, 1.f / 3.f);

  auto result_container_float =
      _cxlumn_statistics_float->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_float.selectivity, 1.f / 3.f);

  auto result_container_double =
      _cxlumn_statistics_double->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_double.selectivity, 1.f / 3.f);

  auto result_container_string =
      _cxlumn_statistics_string->estimate_predicate_with_value_placeholder(predicate_condition);
  EXPECT_FLOAT_EQ(result_container_string.selectivity, 1.f / 3.f);
}

TEST_F(CxlumnStatisticsTest, StoredProcedureBetweenTest) {
  PredicateCondition predicate_condition = PredicateCondition::Between;

  // selectivities = selectivities from LessEqualThan / 0.3f

  std::vector<float> selectivities_int{0.f, 1.f / 18.f, 1.f / 6.f, 1.f / 3.f, 1.f / 3.f};
  predict_selectivities_for_stored_procedures_and_compare(_cxlumn_statistics_int, predicate_condition, _int_values,
                                                          selectivities_int);

  std::vector<float> selectivities_float{0.f, 0.f, 2.f / 15.f, 1.f / 3.f, 1.f / 3.f};
  predict_selectivities_for_stored_procedures_and_compare(_cxlumn_statistics_float, predicate_condition, _float_values,
                                                          selectivities_float);
  predict_selectivities_for_stored_procedures_and_compare(_cxlumn_statistics_double, predicate_condition,
                                                          _double_values, selectivities_float);
}

TEST_F(CxlumnStatisticsTest, TwoCxlumnsEqualsTest) {
  PredicateCondition predicate_condition = PredicateCondition::Equals;

  auto cxlumn_stat1 = std::make_shared<CxlumnStatistics<int>>(0.0f, 10.f, 0, 10);
  auto cxlumn_stat2 = std::make_shared<CxlumnStatistics<int>>(0.0f, 10.f, -10, 20);

  auto result1 = cxlumn_stat1->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat2);
  auto result2 = cxlumn_stat2->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat1);
  float expected_selectivity = (11.f / 31.f) / 10.f;

  EXPECT_FLOAT_EQ(result1.selectivity, expected_selectivity);
  EXPECT_FLOAT_EQ(result2.selectivity, expected_selectivity);

  auto cxlumn_stat3 = std::make_shared<CxlumnStatistics<float>>(0.0f, 10.f, 0.f, 10.f);
  auto cxlumn_stat4 = std::make_shared<CxlumnStatistics<float>>(0.0f, 3.f, -10.f, 20.f);

  auto result3 = cxlumn_stat3->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat4);
  auto result4 = cxlumn_stat4->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat3);
  expected_selectivity = (10.f / 30.f) / 10.f;

  EXPECT_FLOAT_EQ(result3.selectivity, expected_selectivity);
  EXPECT_FLOAT_EQ(result4.selectivity, expected_selectivity);

  auto cxlumn_stat5 = std::make_shared<CxlumnStatistics<float>>(0.0f, 10.f, 20.f, 30.f);

  auto result5 = cxlumn_stat3->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat5);
  auto result6 = cxlumn_stat5->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat3);
  expected_selectivity = 0.f;

  EXPECT_FLOAT_EQ(result5.selectivity, expected_selectivity);
  EXPECT_FLOAT_EQ(result6.selectivity, expected_selectivity);
}

TEST_F(CxlumnStatisticsTest, TwoCxlumnsLessThanTest) {
  PredicateCondition predicate_condition = PredicateCondition::LessThan;

  auto cxlumn_stat1 = std::make_shared<CxlumnStatistics<int>>(0.0f, 10.f, 1, 20);
  auto cxlumn_stat2 = std::make_shared<CxlumnStatistics<int>>(0.0f, 30.f, 11, 40);

  auto result1 = cxlumn_stat1->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat2);
  auto expected_selectivity = ((10.f / 20.f) * (10.f / 30.f) - 0.5f * 1.f / 30.f) * 0.5f + (10.f / 20.f) +
                              (20.f / 30.f) - (10.f / 20.f) * (20.f / 30.f);
  EXPECT_FLOAT_EQ(result1.selectivity, expected_selectivity);

  auto result2 = cxlumn_stat2->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat1);
  expected_selectivity = ((10.f / 20.f) * (10.f / 30.f) - 0.5f * 1.f / 30.f) * 0.5f;
  EXPECT_FLOAT_EQ(result2.selectivity, expected_selectivity);

  auto cxlumn_stat3 = std::make_shared<CxlumnStatistics<float>>(0.0f, 6.f, 0, 10);
  auto cxlumn_stat4 = std::make_shared<CxlumnStatistics<float>>(0.0f, 12.f, -10, 30);

  auto result3 = cxlumn_stat3->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat4);
  expected_selectivity = ((10.f / 10.f) * (10.f / 40.f) - 1.f / (4 * 6)) * 0.5f + (20.f / 40.f);
  EXPECT_FLOAT_EQ(result3.selectivity, expected_selectivity);

  auto result4 = cxlumn_stat4->estimate_predicate_with_cxlumn(predicate_condition, *cxlumn_stat3);
  expected_selectivity = ((10.f / 10.f) * (10.f / 40.f) - 1.f / (4 * 6)) * 0.5f + (10.f / 40.f);
  EXPECT_FLOAT_EQ(result4.selectivity, expected_selectivity);
}

TEST_F(CxlumnStatisticsTest, TwoCxlumnsRealDataTest) {
  // test selectivity calculations for all predicate conditions and all cxlumn combinations of
  // int_equal_distribution.tbl
  std::vector<PredicateCondition> predicate_conditions{PredicateCondition::Equals, PredicateCondition::NotEquals,
                                                       PredicateCondition::LessThan, PredicateCondition::LessThanEquals,
                                                       PredicateCondition::GreaterThan};
  for (auto predicate_condition : predicate_conditions) {
    predict_selectivities_and_compare(_table_uniform_distribution, _cxlumn_statistics_uniform_cxlumns,
                                      predicate_condition);
  }
}

TEST_F(CxlumnStatisticsTest, NonNullRatioOneCxlumnTest) {
  // null value ratio of 0 not tested here, since this is done in all other tests
  _cxlumn_statistics_int->set_null_value_ratio(0.25f);   // non-null value ratio: 0.75
  _cxlumn_statistics_float->set_null_value_ratio(0.5f);  // non-null value ratio: 0.5
  _cxlumn_statistics_string->set_null_value_ratio(1.f);  // non-null value ratio: 0

  auto predicate_condition = PredicateCondition::Equals;
  auto result = _cxlumn_statistics_int->estimate_predicate_with_value(predicate_condition, AllTypeVariant(1));
  EXPECT_FLOAT_EQ(result.selectivity, 0.75f / 6.f);
  result = _cxlumn_statistics_float->estimate_predicate_with_value(predicate_condition, AllTypeVariant(2.f));
  EXPECT_FLOAT_EQ(result.selectivity, 0.5f / 6.f);
  result = _cxlumn_statistics_string->estimate_predicate_with_value(predicate_condition, AllTypeVariant("a"));
  EXPECT_FLOAT_EQ(result.selectivity, 0.f);

  predicate_condition = PredicateCondition::NotEquals;
  result = _cxlumn_statistics_int->estimate_predicate_with_value(predicate_condition, AllTypeVariant(1));
  EXPECT_FLOAT_EQ(result.selectivity, 0.75f * 5.f / 6.f);
  result = _cxlumn_statistics_float->estimate_predicate_with_value(predicate_condition, AllTypeVariant(2.f));
  EXPECT_FLOAT_EQ(result.selectivity, 0.5f * 5.f / 6.f);
  result = _cxlumn_statistics_string->estimate_predicate_with_value(predicate_condition, AllTypeVariant("a"));
  EXPECT_FLOAT_EQ(result.selectivity, 0.f);

  predicate_condition = PredicateCondition::LessThan;
  result = _cxlumn_statistics_int->estimate_predicate_with_value(predicate_condition, AllTypeVariant(3));
  EXPECT_FLOAT_EQ(result.selectivity, 0.75f * 2.f / 6.f);
  result = _cxlumn_statistics_float->estimate_predicate_with_value(predicate_condition, AllTypeVariant(3.f));
  EXPECT_FLOAT_EQ(result.selectivity, 0.5f * 2.f / 5.f);
  result = _cxlumn_statistics_string->estimate_predicate_with_value(predicate_condition, AllTypeVariant("c"));
  EXPECT_FLOAT_EQ(result.selectivity, 0.f);

  predicate_condition = PredicateCondition::GreaterThanEquals;
  result = _cxlumn_statistics_int->estimate_predicate_with_value(predicate_condition, AllTypeVariant(3));
  EXPECT_FLOAT_EQ(result.selectivity, 0.75f * 4.f / 6.f);
  result = _cxlumn_statistics_float->estimate_predicate_with_value(predicate_condition, AllTypeVariant(3.f));
  EXPECT_FLOAT_EQ(result.selectivity, 0.5f * 3.f / 5.f);
  result = _cxlumn_statistics_string->estimate_predicate_with_value(predicate_condition, AllTypeVariant("c"));
  EXPECT_FLOAT_EQ(result.selectivity, 0.f);

  predicate_condition = PredicateCondition::Between;
  result =
      _cxlumn_statistics_int->estimate_predicate_with_value(predicate_condition, AllTypeVariant(2), AllTypeVariant(4));
  EXPECT_FLOAT_EQ(result.selectivity, 0.75f * 3.f / 6.f);
  result = _cxlumn_statistics_float->estimate_predicate_with_value(predicate_condition, AllTypeVariant(4.f),
                                                                   AllTypeVariant(6.f));
  EXPECT_FLOAT_EQ(result.selectivity, 0.5f * 2.f / 5.f);
  result = _cxlumn_statistics_string->estimate_predicate_with_value(predicate_condition, AllTypeVariant("c"),
                                                                    AllTypeVariant("d"));
  EXPECT_FLOAT_EQ(result.selectivity, 0.f);
}

TEST_F(CxlumnStatisticsTest, NonNullRatioTwoCxlumnTest) {
  auto stats_0 =
      std::const_pointer_cast<BaseCxlumnStatistics>(_cxlumn_statistics_uniform_cxlumns[0]);  // values from 0 to 5
  auto stats_1 =
      std::const_pointer_cast<BaseCxlumnStatistics>(_cxlumn_statistics_uniform_cxlumns[1]);  // values from 0 to 2
  auto stats_2 =
      std::const_pointer_cast<BaseCxlumnStatistics>(_cxlumn_statistics_uniform_cxlumns[2]);  // values from 1 to 2

  stats_0->set_null_value_ratio(0.1);   // non-null value ratio: 0.9
  stats_1->set_null_value_ratio(0.2);   // non-null value ratio: 0.8
  stats_2->set_null_value_ratio(0.15);  // non-null value ratio: 0.85

  auto predicate_condition = PredicateCondition::Equals;
  auto result = stats_0->estimate_predicate_with_cxlumn(predicate_condition, *stats_1);
  EXPECT_FLOAT_EQ(result.selectivity, 0.9f * 0.8f * 0.5f / 3.f);

  predicate_condition = PredicateCondition::LessThan;
  result = stats_1->estimate_predicate_with_cxlumn(predicate_condition, *stats_2);
  EXPECT_FLOAT_EQ(result.selectivity, 0.8f * 0.85f * (1.f / 3.f + 1.f / 3.f * 1.f / 2.f));
}

TEST_F(CxlumnStatisticsTest, Dummy) {
  {
    auto dummy_cxlumn_statistics = CxlumnStatistics<int>::dummy();
    EXPECT_EQ(dummy_cxlumn_statistics.min(), 0);
    EXPECT_EQ(dummy_cxlumn_statistics.max(), 0);
  }
  {
    auto dummy_cxlumn_statistics = CxlumnStatistics<float>::dummy();
    EXPECT_EQ(dummy_cxlumn_statistics.min(), 0.0f);
    EXPECT_EQ(dummy_cxlumn_statistics.max(), 0.0f);
  }
  {
    auto dummy_cxlumn_statistics = CxlumnStatistics<double>::dummy();
    EXPECT_EQ(dummy_cxlumn_statistics.min(), 0.0);
    EXPECT_EQ(dummy_cxlumn_statistics.max(), 0.0);
  }
  {
    auto dummy_cxlumn_statistics = CxlumnStatistics<std::string>::dummy();
    EXPECT_EQ(dummy_cxlumn_statistics.min(), "");
    EXPECT_EQ(dummy_cxlumn_statistics.max(), "");
  }
}

}  // namespace opossum
