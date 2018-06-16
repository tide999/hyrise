#include "all_type_variant.hpp"

#include "boost/functional/hash.hpp"
#include "utils/assert.hpp"

#include <cmath>

namespace opossum {

bool is_integral_data_type(const DataType data_type) {
  return data_type == DataType::Long || data_type == DataType::Int;
}

bool is_floating_point_data_type(const DataType data_type){
  return data_type == DataType::Float || data_type == DataType::Double;
}

bool all_type_variant_near(const AllTypeVariant& lhs, const AllTypeVariant& rhs, double max_abs_error) {
  // TODO(anybody) no checks for float vs double etc yet.

  if (lhs.type() == typeid(float) && rhs.type() == typeid(float)) {  //  NOLINT - lint thinks this is a C-style cast
    return std::fabs(boost::get<float>(lhs) - boost::get<float>(rhs)) < max_abs_error;
  } else if (lhs.type() == typeid(double) && rhs.type() == typeid(double)) {  //  NOLINT - see above
    return std::fabs(boost::get<double>(lhs) - boost::get<double>(rhs)) < max_abs_error;
  }

  return lhs == rhs;
}

}  // namespace opossum

namespace std {

size_t hash<opossum::AllTypeVariant>::operator()(const opossum::AllTypeVariant& all_type_variant) const {
  // For null, hash a random number to avoid collisions
  if (all_type_variant.type() == typeid(opossum::NullValue{})) return boost::hash_value(0xf569a38f);
  if (all_type_variant.type() == typeid(int32_t)) return boost::hash_value(static_cast<int32_t>(boost::get<int32_t>(all_type_variant)));
  if (all_type_variant.type() == typeid(int64_t)) return boost::hash_value(boost::get<int64_t>(all_type_variant));
  if (all_type_variant.type() == typeid(float)) return boost::hash_value(boost::get<float>(all_type_variant));
  if (all_type_variant.type() == typeid(double)) return boost::hash_value(boost::get<double>(all_type_variant));
  if (all_type_variant.type() == typeid(std::string)) return boost::hash_value(boost::get<std::string>(all_type_variant));
  opossum::Fail("Unhandled type");
}

}  // namespace std
