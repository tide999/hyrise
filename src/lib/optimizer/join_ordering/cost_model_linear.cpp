#include <operators/abstract_operator.hpp>

#include <chrono>

#include "abstract_cost_feature_proxy.hpp"
#include "cost_model_linear.hpp"
#include "operators/join_hash.hpp"
#include "operators/union_positions.hpp"
#include "operators/table_scan.hpp"
#include "utils/assert.hpp"

namespace opossum {

CostModelLinearConfig CostModelLinear::create_debug_build_config() {
  CostModelLinearConfig config;

  config.table_scan_models[CostModelLinearTableScanType::ColumnValueNumeric] = CostFeatureWeights{
    {CostFeature::LeftInputReferenceRowCount, 0.151131599208f},
    {CostFeature::LeftInputRowCount, 0.15565695035f},
    {CostFeature::OutputRowCount, 0.112082904016f},
    {CostFeature::OutputDereferenceRowCount, 0.0248249284018f}
  };
  config.table_scan_models[CostModelLinearTableScanType::ColumnColumnNumeric] = CostFeatureWeights{
    {CostFeature::LeftInputReferenceRowCount, 0.0f},
    {CostFeature::LeftInputRowCount, 0.265919959986f},
    {CostFeature::OutputRowCount, 0.0f},
    {CostFeature::OutputDereferenceRowCount, 0.0f}
  };
  config.table_scan_models[CostModelLinearTableScanType::ColumnValueString] = CostFeatureWeights{
    {CostFeature::LeftInputReferenceRowCount, 0.389646092813f},
    {CostFeature::LeftInputRowCount, 0.150529434248f},
    {CostFeature::OutputRowCount, 0.140562838787f},
    {CostFeature::OutputDereferenceRowCount, 0.0576460191188f},
  };
  config.table_scan_models[CostModelLinearTableScanType::ColumnColumnString] = CostFeatureWeights{
    {CostFeature::LeftInputReferenceRowCount, 0.389646092813f},
    {CostFeature::LeftInputRowCount, 0.150529434248f},
    {CostFeature::OutputRowCount, 0.140562838787f},
    {CostFeature::OutputDereferenceRowCount, 0.0576460191188f},
  };
  config.table_scan_models[CostModelLinearTableScanType::Like] = CostFeatureWeights{
    {CostFeature::LeftInputReferenceRowCount, 0.0f},
    {CostFeature::LeftInputRowCount, 8.0548057056f},
    {CostFeature::OutputRowCount, 0.0f},
    {CostFeature::OutputDereferenceRowCount, 0.0f},
  };

  config.other_operator_models[OperatorType::JoinHash] = {
    {CostFeature::MajorInputRowCount, 0.9954f},
    {CostFeature::MajorInputReferenceRowCount, 4.0f},
    {CostFeature::MinorInputRowCount, 11.7f},
    {CostFeature::MinorInputReferenceRowCount, 1.5f},
    {CostFeature::OutputRowCount, 3.0f},
  };

  config.other_operator_models[OperatorType::Product] = {
    {CostFeature::OutputRowCount, 0.0493615676317f}
  };

  config.other_operator_models[OperatorType::UnionPositions] = {
    {CostFeature::LeftInputRowCountLogN, 0.2f},
    {CostFeature::RightInputRowCountLogN, 0.2f},
    {CostFeature::OutputRowCount, 0.4f}
  };

  return config;
}

CostModelLinearConfig CostModelLinear::create_release_build_config() {
  CostModelLinearConfig config;

  config.table_scan_models[CostModelLinearTableScanType::ColumnValueNumeric] = CostFeatureWeights{
  {CostFeature::LeftInputReferenceRowCount, 0.00357555320143f},
  {CostFeature::LeftInputRowCount, 0.0189155666155f},
  {CostFeature::OutputRowCount, 0.00185955501541f},
  {CostFeature::OutputDereferenceRowCount, 0.0f}
  };
  config.table_scan_models[CostModelLinearTableScanType::ColumnColumnNumeric] = CostFeatureWeights{
  {CostFeature::LeftInputReferenceRowCount, 0.0f},
  {CostFeature::LeftInputRowCount, 0.0262691992411f},
  {CostFeature::OutputRowCount, 0.0f},
  {CostFeature::OutputDereferenceRowCount, 0.0f}
  };
  config.table_scan_models[CostModelLinearTableScanType::ColumnValueString] = CostFeatureWeights{
  {CostFeature::LeftInputReferenceRowCount, 0.0122686866294f},
  {CostFeature::LeftInputRowCount, 0.0183285792719f},
  {CostFeature::OutputRowCount, 0.00602195854223f},
  {CostFeature::OutputDereferenceRowCount, 0.0325447573152f},
  };
  config.table_scan_models[CostModelLinearTableScanType::ColumnColumnString] = CostFeatureWeights{
  {CostFeature::LeftInputReferenceRowCount, 0.0122686866294f},
  {CostFeature::LeftInputRowCount, 0.0183285792719f},
  {CostFeature::OutputRowCount, 0.00602195854223f},
  {CostFeature::OutputDereferenceRowCount, 0.0325447573152f},
  };
  config.table_scan_models[CostModelLinearTableScanType::Like] = CostFeatureWeights{
  {CostFeature::LeftInputReferenceRowCount, 1.96376974778f},
  {CostFeature::LeftInputRowCount, 0.0f},
  {CostFeature::OutputRowCount, 11.2450352145f},
  {CostFeature::OutputDereferenceRowCount, 0.0f},
  };

  config.other_operator_models[OperatorType::JoinHash] = {
  {CostFeature::MajorInputRowCount, 0.077f},
  {CostFeature::MajorInputReferenceRowCount, 0.7f},
  {CostFeature::MinorInputRowCount, 1.5f},
  {CostFeature::MinorInputReferenceRowCount, 0.4f},
  {CostFeature::OutputRowCount, 0.25f},
  };

  config.other_operator_models[OperatorType::Product] = {
  {CostFeature::OutputRowCount, 0.0136870174701f}
  };

  config.other_operator_models[OperatorType::UnionPositions] = {
  {CostFeature::LeftInputRowCountLogN, 0.0025f},
  {CostFeature::RightInputRowCountLogN, 0.0025f},
  {CostFeature::OutputRowCount, 0.0025f}
  };

  return config;
}

CostModelLinearConfig CostModelLinear::create_current_build_type_config() {
#if IS_DEBUG
  return create_debug_build_config();
#else
  return create_release_build_config();
#endif
}

CostModelLinear::CostModelLinear(const CostModelLinearConfig& config):
  _config(config) {

}

std::string CostModelLinear::name() const {
  return "CostModelLinear";
}

Cost CostModelLinear::get_reference_operator_cost(const std::shared_ptr<AbstractOperator>& op) const {
  Assert(op->get_output(), "Can only get reference cost of Operators that were executed");

  auto duration = std::chrono::microseconds{0};

  switch (op->type()) {
    case OperatorType::JoinHash: {
      const auto join_hash = std::static_pointer_cast<JoinHash>(op);
      const auto &performance_data = join_hash->performance_data();
      duration = performance_data.materialization + performance_data.partitioning + performance_data.build +
                 performance_data.probe;
    } break;

    default:
      duration = op->performance_data().total;
  }

  return static_cast<Cost>(duration.count());
}

Cost CostModelLinear::_cost_model_impl(const OperatorType operator_type, const AbstractCostFeatureProxy& feature_proxy) const {
  switch (operator_type) {
    case OperatorType::TableScan: {
      const auto predicate_condition = feature_proxy.extract_feature(CostFeature::PredicateCondition).predicate_condition();
      const auto left_data_type = feature_proxy.extract_feature(CostFeature::LeftDataType).data_type();
      const auto right_data_type = feature_proxy.extract_feature(CostFeature::RightDataType).data_type();

      auto feature_weights_iter = _config.table_scan_models.end();

      if (predicate_condition == PredicateCondition::Like || predicate_condition == PredicateCondition::NotLike) {
        Assert(left_data_type == DataType::String && right_data_type == DataType::String, "Expected string for LIKE");
        feature_weights_iter = _config.table_scan_models.find(CostModelLinearTableScanType::Like);
      } else {
        const auto right_operand_is_column = feature_proxy.extract_feature(CostFeature::RightOperandIsColumn).boolean();

        if (left_data_type == DataType::String) {
          if (right_operand_is_column) {
            feature_weights_iter = _config.table_scan_models.find(CostModelLinearTableScanType::ColumnColumnString);
          } else {
            feature_weights_iter = _config.table_scan_models.find(CostModelLinearTableScanType::ColumnValueString);
          }
        } else {
          if (right_operand_is_column) {
            feature_weights_iter = _config.table_scan_models.find(CostModelLinearTableScanType::ColumnColumnNumeric);
          } else {
            feature_weights_iter = _config.table_scan_models.find(CostModelLinearTableScanType::ColumnValueNumeric);
          }
        }
      }

      Assert(feature_weights_iter != _config.table_scan_models.end(), "TableScanModel not found");

      return _predict_cost(feature_weights_iter->second, feature_proxy);
    }

    default: {
      const auto feature_weights_iter = _config.other_operator_models.find(operator_type);
      if (feature_weights_iter == _config.other_operator_models.end()) return 0;

      return _predict_cost(feature_weights_iter->second, feature_proxy);
    }
  }
}


Cost CostModelLinear::_predict_cost(const CostFeatureWeights& feature_weights, const AbstractCostFeatureProxy& feature_proxy) const {
  auto cost = Cost{0};

  for (const auto& feature_and_weight : feature_weights) {
    cost += feature_proxy.extract_feature(feature_and_weight.first).scalar() * feature_and_weight.second;
  }

  return cost;
}

}  // namespace opossum