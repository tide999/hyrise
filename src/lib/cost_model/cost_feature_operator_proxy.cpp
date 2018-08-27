#include "cost_feature_operator_proxy.hpp"

#include "operators/abstract_operator.hpp"
#include "operators/join_hash.hpp"
#include "operators/join_nested_loop.hpp"
#include "operators/join_sort_merge.hpp"
#include "operators/product.hpp"
#include "operators/table_scan.hpp"
#include "resolve_type.hpp"
#include "storage/table.hpp"

namespace opossum {

CostFeatureOperatorProxy::CostFeatureOperatorProxy(const std::shared_ptr<AbstractOperator>& op) : _op(op) {}

CostFeatureVariant CostFeatureOperatorProxy::_extract_feature_impl(const CostFeature cost_feature) const {
  switch (cost_feature) {
    case CostFeature::LeftInputRowCount:
      Assert(_op->input_table_left(), "Can't extract CostFeature since the input table is not available");
      return static_cast<float>(_op->input_table_left()->row_count());
    case CostFeature::RightInputRowCount:
      Assert(_op->input_table_right(), "Can't extract CostFeature since the input table is not available");
      return static_cast<float>(_op->input_table_right()->row_count());
    case CostFeature::LeftInputIsReferences:
      Assert(_op->input_table_left(), "Can't extract CostFeature since the input table is not available");
      return _op->input_table_left()->type() == TableType::References;
    case CostFeature::RightInputIsReferences:
      Assert(_op->input_table_right(), "Can't extract CostFeature since the input table is not available");
      return _op->input_table_right()->type() == TableType::References;
    case CostFeature::OutputRowCount:
      Assert(_op->get_output(), "Can't extract CostFeature since the output table is not available");
      return static_cast<float>(_op->get_output()->row_count());

    case CostFeature::LeftDataType:
    case CostFeature::RightDataType: {
      if (const auto join_op = std::dynamic_pointer_cast<AbstractJoinOperator>(_op); join_op) {
        if (cost_feature == CostFeature::LeftDataType) {
          Assert(_op->input_table_left(), "Input operator must be executed");
          return join_op->input_table_left()->cxlumn_data_type(join_op->cxlumn_ids().first);
        } else {
          Assert(_op->input_table_right(), "Input operator must be executed");
          return join_op->input_table_right()->cxlumn_data_type(join_op->cxlumn_ids().second);
        }
      } else if (_op->type() == OperatorType::TableScan) {
        Assert(_op->input_table_left(), "Input operator must be executed");

        const auto table_scan = std::dynamic_pointer_cast<TableScan>(_op);
        if (cost_feature == CostFeature::LeftDataType) {
          return table_scan->input_table_left()->cxlumn_data_type(table_scan->left_cxlumn_id());
        } else {
          if (table_scan->right_parameter().type() == typeid(AllTypeVariant)) {
            return data_type_from_all_type_variant(boost::get<AllTypeVariant>(table_scan->right_parameter()));
          } else {
            Assert(is_cxlumn_id(table_scan->right_parameter()), "Expected LQPCxlumnReference");
            const auto cxlumn_id = boost::get<CxlumnID>(table_scan->right_parameter());
            return table_scan->input_table_left()->cxlumn_data_type(cxlumn_id);
          }
        }
      } else {
        Fail("This CostFeature is not defined for this LQPNodeType");
      }
    }

    case CostFeature::PredicateCondition:
      if (const auto join_op = std::dynamic_pointer_cast<AbstractJoinOperator>(_op); join_op) {
        return join_op->predicate_condition();
      } else if (_op->type() == OperatorType::TableScan) {
        return std::static_pointer_cast<TableScan>(_op)->predicate_condition();
      } else {
        Fail("This CostFeature is not defined for this LQPNodeType");
      }

    case CostFeature::RightOperandIsCxlumn:
      if (_op->type() == OperatorType::TableScan) {
        return is_cxlumn_id(std::static_pointer_cast<TableScan>(_op)->right_parameter());
      } else {
        Fail("CostFeature not defined for LQPNodeType");
      }

    default:
      Fail("Extraction of this feature is not implemented. Maybe it should be handled in AbstractCostFeatureProxy?");
  }
}

}  // namespace opossum
