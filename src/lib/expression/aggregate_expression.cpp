#include "aggregate_expression.hpp"

#include <sstream>

#include "boost/functional/hash.hpp"

#include "utils/assert.hpp"
#include "constant_mappings.hpp"
#include "expression_utils.hpp"
#include "operators/aggregate/aggregate_traits.hpp"
#include "resolve_type.hpp"

namespace opossum {

AggregateExpression::AggregateExpression(const AggregateFunction aggregate_function):
  AbstractExpression(ExpressionType::Aggregate, {}), aggregate_function(aggregate_function) {
  Assert(aggregate_function == AggregateFunction::Count || aggregate_function == AggregateFunction::CountDistinct, "Only COUNT aggregates can have no arguments");
}

AggregateExpression::AggregateExpression(const AggregateFunction aggregate_function,
                                         const std::shared_ptr<AbstractExpression>& argument):
  AbstractExpression(ExpressionType::Aggregate, {argument}), aggregate_function(aggregate_function) {}

std::shared_ptr<AbstractExpression> AggregateExpression::argument() const {
  return arguments.empty() ? nullptr : arguments[0];
}

std::shared_ptr<AbstractExpression> AggregateExpression::deep_copy() const {
  if (argument()) {
    return std::make_shared<AggregateExpression>(aggregate_function, argument()->deep_copy());
  } else {
    return std::make_shared<AggregateExpression>(aggregate_function);
  }
}

std::string AggregateExpression::as_column_name() const {
  std::stringstream stream;

  if (aggregate_function == AggregateFunction::CountDistinct) {
    Assert(argument(), "COUNT(DISTINCT ...) requires a argument");
    stream << "COUNT(DISTINCT " << _argument_as_column_name(*argument()) << ")";
  } else if (aggregate_function == AggregateFunction::Count && !argument()) {
    stream << "COUNT(*)";
  } else {
    stream << aggregate_function_to_string.left.at(aggregate_function) << "(";
    if (argument()) stream << _argument_as_column_name(*argument());
    stream << ")";
  }

  return stream.str();
}

DataType AggregateExpression::data_type() const {
  if (aggregate_function == AggregateFunction::Count) return AggregateTraits<NullValue, AggregateFunction::Count>::aggregate_data_type;
  if (aggregate_function == AggregateFunction::CountDistinct) return AggregateTraits<NullValue, AggregateFunction::CountDistinct>::aggregate_data_type;

  Assert(arguments.size() == 1, "Expected this AggregateFunction to have one argument");

  const auto argument_data_type = arguments[0]->data_type();
  auto aggregate_data_type = DataType::Null;

  resolve_data_type(argument_data_type, [&](const auto data_type_t) {
    using AggregateDataType = typename decltype(data_type_t)::type;
    switch (aggregate_function) {
      case AggregateFunction::Min:
        aggregate_data_type = AggregateTraits<AggregateDataType, AggregateFunction::Min>::aggregate_data_type;
        break;
      case AggregateFunction::Max:
        aggregate_data_type = AggregateTraits<AggregateDataType, AggregateFunction::Max>::aggregate_data_type;
        break;
      case AggregateFunction::Avg:
        aggregate_data_type = AggregateTraits<AggregateDataType, AggregateFunction::Avg>::aggregate_data_type;
        break;
      case AggregateFunction::Count:
      case AggregateFunction::CountDistinct:
        break;  // These are handled above
      case AggregateFunction::Sum:
        aggregate_data_type = AggregateTraits<AggregateDataType, AggregateFunction::Sum>::aggregate_data_type;
        break;
    }
  });

  return aggregate_data_type;
}

bool AggregateExpression::_shallow_equals(const AbstractExpression& expression) const {
  return aggregate_function == static_cast<const AggregateExpression&>(expression).aggregate_function;
}

size_t AggregateExpression::_on_hash() const {
  return boost::hash_value(static_cast<size_t>(aggregate_function));
}

}  // namespace opossum
