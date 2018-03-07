#include "table.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "resolve_type.hpp"
#include "storage/partitioning/hash_partition_schema.hpp"
#include "storage/partitioning/null_partition_schema.hpp"
#include "storage/partitioning/range_partition_schema.hpp"
#include "storage/partitioning/round_robin_partition_schema.hpp"
#include "types.hpp"
#include "utils/assert.hpp"
#include "value_column.hpp"

namespace opossum {

std::shared_ptr<Table> Table::create_dummy_table(const TableColumnDefinitions& column_definitions) {
  return std::make_shared<Table>(column_definitions, TableType::Data);
}

Table::Table(const TableColumnDefinitions& column_definitions, const TableType type, const uint32_t max_chunk_size,
             const UseMvcc use_mvcc)
    : _column_definitions(column_definitions),
      _type(type),
      _use_mvcc(use_mvcc),
      _max_chunk_size(max_chunk_size),
      _append_mutex(std::make_unique<std::mutex>()) {
  Assert(max_chunk_size > 0, "Table must have a chunk size greater than 0.");
}

const TableColumnDefinitions& Table::column_definitions() const { return _column_definitions; }

TableType Table::type() const { return _type; }

UseMvcc Table::has_mvcc() const { return _use_mvcc; }

size_t Table::column_count() const { return _column_definitions.size(); }

const std::string& Table::column_name(const ColumnID column_id) const {
  DebugAssert(column_id < _column_definitions.size(), "ColumnID out of range");
  return _column_definitions[column_id].name;
}

<<<<<<< HEAD
Table::Table(const uint32_t max_chunk_size)
    : _max_chunk_size(max_chunk_size), _append_mutex(std::make_unique<std::mutex>()) {
  Assert(max_chunk_size > 0, "Table must have a chunk size greater than 0.");
  apply_partitioning(std::make_shared<NullPartitionSchema>());
=======
std::vector<std::string> Table::column_names() const {
  std::vector<std::string> names;
  names.reserve(_column_definitions.size());
  for (const auto& column_definition : _column_definitions) {
    names.emplace_back(column_definition.name);
  }
  return names;
>>>>>>> e439647208e88b581d7c32231341439a3b5feb97
}

DataType Table::column_data_type(const ColumnID column_id) const {
  DebugAssert(column_id < _column_definitions.size(), "ColumnID out of range");
  return _column_definitions[column_id].data_type;
}

std::vector<DataType> Table::column_data_types() const {
  std::vector<DataType> data_types;
  data_types.reserve(_column_definitions.size());
  for (const auto& column_definition : _column_definitions) {
    data_types.emplace_back(column_definition.data_type);
  }
  return data_types;
}

bool Table::column_is_nullable(const ColumnID column_id) const {
  DebugAssert(column_id < _column_definitions.size(), "ColumnID out of range");
  return _column_definitions[column_id].nullable;
}

std::vector<bool> Table::columns_are_nullable() const {
  std::vector<bool> nullable(column_count());
  for (size_t column_idx = 0; column_idx < column_count(); ++column_idx) {
    nullable[column_idx] = _column_definitions[column_idx].nullable;
  }
  return nullable;
}

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  const auto iter = std::find_if(_column_definitions.begin(), _column_definitions.end(),
                                 [&](const auto& column_definition) { return column_definition.name == column_name; });
  Assert(iter != _column_definitions.end(), "Couldn't find column '" + column_name + "'");
  return static_cast<ColumnID>(std::distance(_column_definitions.begin(), iter));
}

<<<<<<< HEAD
void Table::append(const std::vector<AllTypeVariant>& values) {
  // TODO(Anyone): Chunks should be preallocated for chunk size
  auto partition_id = _partition_schema->get_matching_partition_for(values);
  auto last_chunk = _partition_schema->last_chunk(partition_id);
  if (last_chunk->size() >= max_chunk_size()) {
    last_chunk = create_new_chunk(partition_id);
  }
  last_chunk->append(values);
}

std::shared_ptr<Chunk> Table::create_new_chunk(PartitionID partition_id) {
  // Create chunk with mvcc columns
  auto new_chunk = std::make_shared<Chunk>(UseMvcc::Yes);

  for (auto column_id = 0u; column_id < _column_types.size(); ++column_id) {
    const auto type = _column_types[column_id];
    auto nullable = _column_nullable[column_id];

    new_chunk->add_column(make_shared_by_data_type<BaseColumn, ValueColumn>(type, nullable));
  }

  new_chunk->set_id(static_cast<ChunkID>(_chunks.size()));
  _chunks.push_back(new_chunk);
  _partition_schema->add_new_chunk(new_chunk, partition_id);

  return new_chunk;
=======
void Table::append(std::vector<AllTypeVariant> values) {
  if (_chunks.empty() || _chunks.back()->size() >= _max_chunk_size) {
    append_mutable_chunk();
  }

  _chunks.back()->append(values);
}

void Table::append_mutable_chunk() {
  ChunkColumns columns;
  for (const auto& column_definition : _column_definitions) {
    resolve_data_type(column_definition.data_type, [&](auto type) {
      using ColumnDataType = typename decltype(type)::type;
      columns.push_back(std::make_shared<ValueColumn<ColumnDataType>>(column_definition.nullable));
    });
  }
  append_chunk(columns);
>>>>>>> e439647208e88b581d7c32231341439a3b5feb97
}

uint64_t Table::row_count() const {
  uint64_t ret = 0;
  for (const auto& chunk : _chunks) {
    ret += chunk->size();
  }
  return ret;
}

bool Table::empty() const { return row_count() == 0u; }

ChunkID Table::chunk_count() const { return static_cast<ChunkID>(_chunks.size()); }

const std::vector<std::shared_ptr<Chunk>>& Table::chunks() const { return _chunks; }

uint32_t Table::max_chunk_size() const { return _max_chunk_size; }

<<<<<<< HEAD
const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(ColumnID column_id) const {
  DebugAssert(column_id < _column_names.size(), "ColumnID " + std::to_string(column_id) + " out of range");
  return _column_names[column_id];
}

DataType Table::column_type(ColumnID column_id) const {
  DebugAssert(column_id < _column_names.size(), "ColumnID " + std::to_string(column_id) + " out of range");
  return _column_types[column_id];
}

bool Table::column_is_nullable(ColumnID column_id) const {
  DebugAssert(column_id < _column_names.size(), "ColumnID " + std::to_string(column_id) + " out of range");
  return _column_nullable[column_id];
}

const std::vector<DataType>& Table::column_types() const { return _column_types; }

const std::vector<bool>& Table::column_nullables() const { return _column_nullable; }

std::shared_ptr<Chunk> Table::get_mutable_chunk(ChunkID chunk_id) {
=======
std::shared_ptr<Chunk> Table::get_chunk(ChunkID chunk_id) {
>>>>>>> e439647208e88b581d7c32231341439a3b5feb97
  DebugAssert(chunk_id < _chunks.size(), "ChunkID " + std::to_string(chunk_id) + " out of range");
  return _chunks[chunk_id];
}

std::shared_ptr<const Chunk> Table::get_chunk(ChunkID chunk_id) const {
  DebugAssert(chunk_id < _chunks.size(), "ChunkID " + std::to_string(chunk_id) + " out of range");
  return _chunks[chunk_id];
}

ProxyChunk Table::get_mutable_chunk_with_access_counting(ChunkID chunk_id) {
  DebugAssert(chunk_id < _chunks.size(), "ChunkID " + std::to_string(chunk_id) + " out of range");
  return ProxyChunk(_chunks[chunk_id]);
}

const ProxyChunk Table::get_chunk_with_access_counting(ChunkID chunk_id) const {
  DebugAssert(chunk_id < _chunks.size(), "ChunkID " + std::to_string(chunk_id) + " out of range");
  return ProxyChunk(_chunks[chunk_id]);
}

<<<<<<< HEAD
void Table::emplace_chunk(const std::shared_ptr<Chunk>& chunk, PartitionID partition_id) {
  if (_chunks.size() == 1 && (_chunks.back()->column_count() == 0 || _chunks.back()->size() == 0)) {
    // the initial chunk was not used yet
    _chunks.clear();
    _partition_schema->clear();
  }
  DebugAssert(chunk->column_count() > 0, "Trying to add chunk without columns.");
  DebugAssert(chunk->column_count() == column_count(),
              std::string("adding chunk with ") + std::to_string(chunk->column_count()) + " columns to table with " +
                  std::to_string(column_count()) + " columns");
  chunk->set_id(static_cast<ChunkID>(_chunks.size()));
  _chunks.emplace_back(chunk);
  _partition_schema->add_new_chunk(chunk, partition_id);
}

std::unique_lock<std::mutex> Table::acquire_append_mutex() { return std::unique_lock<std::mutex>(*_append_mutex); }
=======
void Table::append_chunk(const ChunkColumns& columns, const std::optional<PolymorphicAllocator<Chunk>>& alloc,
                         const std::shared_ptr<ChunkAccessCounter>& access_counter) {
  const auto chunk_size = columns.empty() ? 0u : columns[0]->size();
>>>>>>> e439647208e88b581d7c32231341439a3b5feb97

#if IS_DEBUG
  for (const auto& column : columns) {
    DebugAssert(column->size() == chunk_size, "Columns don't have the same length");
    const auto is_reference_column = std::dynamic_pointer_cast<ReferenceColumn>(column) != nullptr;
    switch (_type) {
      case TableType::References:
        DebugAssert(is_reference_column, "Invalid column type");
        break;
      case TableType::Data:
        DebugAssert(!is_reference_column, "Invalid column type");
        break;
    }
  }
#endif

  std::shared_ptr<MvccColumns> mvcc_columns;

  if (_use_mvcc == UseMvcc::Yes) {
    mvcc_columns = std::make_shared<MvccColumns>(chunk_size);
  }

  _chunks.emplace_back(std::make_shared<Chunk>(columns, mvcc_columns, alloc, access_counter));
}

<<<<<<< HEAD
void Table::apply_partitioning(const std::shared_ptr<AbstractPartitionSchema> partition_schema) {
  if (row_count() > 0) {
    throw std::runtime_error("Unable to create partitioning on non-empty table");
  }
  _partition_schema = partition_schema;
  _create_initial_chunks(static_cast<PartitionID>(partition_schema->partition_count()));
}

void Table::set_partitioning_and_clear(const std::shared_ptr<AbstractPartitionSchema> partition_schema) {
  _partition_schema = std::move(partition_schema);
  _chunks.clear();
}

void Table::_create_initial_chunks(PartitionID number_of_partitions) {
  if (row_count() > 0) {
    // the initial chunk was not used yet
    throw std::runtime_error("Unable to create partitioning on non-empty table");
  }
  _chunks.clear();
  for (auto partition_id = PartitionID{0}; partition_id < number_of_partitions; ++partition_id) {
    create_new_chunk(partition_id);
  }
}

bool Table::is_partitioned() const { return _partition_schema->is_partitioned(); }

PartitionID Table::partition_count() const { return _partition_schema->partition_count(); }

const std::shared_ptr<const AbstractPartitionSchema> Table::get_partition_schema() const { return _partition_schema; }
=======
std::unique_lock<std::mutex> Table::acquire_append_mutex() { return std::unique_lock<std::mutex>(*_append_mutex); }
>>>>>>> e439647208e88b581d7c32231341439a3b5feb97

std::vector<IndexInfo> Table::get_indexes() const { return _indexes; }

size_t Table::estimate_memory_usage() const {
  auto bytes = size_t{sizeof(*this)};

  for (const auto& chunk : _chunks) {
    bytes += chunk->estimate_memory_usage();
  }

  for (const auto& column_definition : _column_definitions) {
    bytes += column_definition.name.size();
  }

  // TODO(anybody) Statistics and Indices missing from Memory Usage Estimation
  // TODO(anybody) TableLayout missing

  return bytes;
}

}  // namespace opossum
