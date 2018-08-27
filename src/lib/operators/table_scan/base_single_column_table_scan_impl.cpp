#include "base_single_column_table_scan_impl.hpp"

#include <memory>
#include <unordered_map>
#include <utility>

#include "resolve_type.hpp"
#include "storage/chunk.hpp"
#include "storage/segment_iterables/chunk_offset_mapping.hpp"
#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"
#include "storage/table.hpp"
#include "storage/value_segment.hpp"

namespace opossum {

BaseSingleColumnTableScanImpl::BaseSingleColumnTableScanImpl(const std::shared_ptr<const Table>& in_table,
                                                             const CxlumnID cxlumn_id,
                                                             const PredicateCondition predicate_condition)
    : BaseTableScanImpl{in_table, cxlumn_id, predicate_condition} {}

std::shared_ptr<PosList> BaseSingleColumnTableScanImpl::scan_chunk(ChunkID chunk_id) {
  const auto chunk = _in_table->get_chunk(chunk_id);
  const auto column = chunk->get_segment(_left_cxlumn_id);

  auto matches_out = std::make_shared<PosList>();
  auto context = std::make_shared<Context>(chunk_id, *matches_out);

  resolve_data_and_cxlumn_type(*column, [&](const auto data_type_t, const auto& resolved_column) {
    static_cast<AbstractSegmentVisitor*>(this)->handle_segment(resolved_column, context);
  });

  return matches_out;
}

void BaseSingleColumnTableScanImpl::handle_segment(const ReferenceSegment& column,
                                                  std::shared_ptr<SegmentVisitorContext> base_context) {
  auto context = std::static_pointer_cast<Context>(base_context);
  const ChunkID chunk_id = context->_chunk_id;
  auto& matches_out = context->_matches_out;

  auto chunk_offsets_by_chunk_id = split_pos_list_by_chunk_id(*column.pos_list());

  // Visit each referenced column
  for (auto& pair : chunk_offsets_by_chunk_id) {
    const auto& referenced_chunk_id = pair.first;
    auto& mapped_chunk_offsets = pair.second;

    const auto chunk = column.referenced_table()->get_chunk(referenced_chunk_id);
    auto referenced_column = chunk->get_segment(column.referenced_cxlumn_id());

    auto mapped_chunk_offsets_ptr = std::make_unique<ChunkOffsetsList>(std::move(mapped_chunk_offsets));

    auto new_context = std::make_shared<Context>(chunk_id, matches_out, std::move(mapped_chunk_offsets_ptr));

    resolve_data_and_cxlumn_type(*referenced_column, [&](const auto data_type_t, const auto& resolved_column) {
      static_cast<AbstractSegmentVisitor*>(this)->handle_segment(resolved_column, new_context);
    });
  }
}

}  // namespace opossum
