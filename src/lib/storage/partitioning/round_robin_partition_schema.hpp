#pragma once

#include "all_type_variant.hpp"
#include "storage/partitioning/partition_schema.hpp"
#include "types.hpp"

namespace opossum {

class RoundRobinPartitionSchema : public PartitionSchema {
 public:
  explicit RoundRobinPartitionSchema(size_t number_of_partitions);

  void append(std::vector<AllTypeVariant> values, const uint32_t max_chunk_size,
              const std::vector<DataType>& column_types, const std::vector<bool>& column_nullables) override;

  RoundRobinPartitionSchema(RoundRobinPartitionSchema&&) = default;
  RoundRobinPartitionSchema& operator=(RoundRobinPartitionSchema&&) = default;

  const PartitionID get_matching_partition_for(AllTypeVariant value) const override;

 protected:
  int _number_of_partitions;
  PartitionID _next_partition;
};

}  // namespace opossum
