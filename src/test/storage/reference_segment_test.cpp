#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "operators/abstract_operator.hpp"
#include "operators/get_table.hpp"
#include "operators/print.hpp"
#include "operators/table_scan.hpp"
#include "storage/chunk_encoder.hpp"
#include "storage/reference_segment.hpp"
#include "storage/storage_manager.hpp"
#include "storage/table.hpp"
#include "types.hpp"

namespace opossum {

class ReferenceSegmentTest : public BaseTest {
  virtual void SetUp() {
    TableCxlumnDefinitions cxlumn_definitions;
    cxlumn_definitions.emplace_back("a", DataType::Int, true);
    cxlumn_definitions.emplace_back("b", DataType::Float);

    _test_table = std::make_shared<opossum::Table>(cxlumn_definitions, TableType::Data, 3);
    _test_table->append({123, 456.7f});
    _test_table->append({1234, 457.7f});
    _test_table->append({12345, 458.7f});
    _test_table->append({NULL_VALUE, 458.7f});
    _test_table->append({12345, 458.7f});

    TableCxlumnDefinitions cxlumn_definitions2;
    cxlumn_definitions2.emplace_back("a", DataType::Int);
    cxlumn_definitions2.emplace_back("b", DataType::Int);
    _test_table_dict = std::make_shared<opossum::Table>(cxlumn_definitions2, TableType::Data, 5, UseMvcc::Yes);
    for (int i = 0; i <= 24; i += 2) _test_table_dict->append({i, 100 + i});

    ChunkEncoder::encode_chunks(_test_table_dict, {ChunkID{0}, ChunkID{1}});

    StorageManager::get().add_table("test_table_dict", _test_table_dict);
  }

 public:
  std::shared_ptr<opossum::Table> _test_table, _test_table_dict;
};

TEST_F(ReferenceSegmentTest, IsImmutable) {
  auto pos_list =
      std::make_shared<PosList>(std::initializer_list<RowID>({{ChunkID{0}, 0}, {ChunkID{0}, 1}, {ChunkID{0}, 2}}));
  auto ref_column = ReferenceSegment(_test_table, CxlumnID{0}, pos_list);

  EXPECT_THROW(ref_column.append(1), std::logic_error);
}

TEST_F(ReferenceSegmentTest, RetrievesValues) {
  // PosList with (0, 0), (0, 1), (0, 2)
  auto pos_list = std::make_shared<PosList>(
      std::initializer_list<RowID>({RowID{ChunkID{0}, 0}, RowID{ChunkID{0}, 1}, RowID{ChunkID{0}, 2}}));
  auto ref_column = ReferenceSegment(_test_table, CxlumnID{0}, pos_list);

  auto& column = *(_test_table->get_chunk(ChunkID{0})->get_column(CxlumnID{0}));

  EXPECT_EQ(ref_column[0], column[0]);
  EXPECT_EQ(ref_column[1], column[1]);
  EXPECT_EQ(ref_column[2], column[2]);
}

TEST_F(ReferenceSegmentTest, RetrievesValuesOutOfOrder) {
  // PosList with (0, 1), (0, 2), (0, 0)
  auto pos_list = std::make_shared<PosList>(
      std::initializer_list<RowID>({RowID{ChunkID{0}, 1}, RowID{ChunkID{0}, 2}, RowID{ChunkID{0}, 0}}));
  auto ref_column = ReferenceSegment(_test_table, CxlumnID{0}, pos_list);

  auto& column = *(_test_table->get_chunk(ChunkID{0})->get_column(CxlumnID{0}));

  EXPECT_EQ(ref_column[0], column[1]);
  EXPECT_EQ(ref_column[1], column[2]);
  EXPECT_EQ(ref_column[2], column[0]);
}

TEST_F(ReferenceSegmentTest, RetrievesValuesFromChunks) {
  // PosList with (0, 2), (1, 0), (1, 1)
  auto pos_list = std::make_shared<PosList>(
      std::initializer_list<RowID>({RowID{ChunkID{0}, 2}, RowID{ChunkID{1}, 0}, RowID{ChunkID{1}, 1}}));
  auto ref_column = ReferenceSegment(_test_table, CxlumnID{0}, pos_list);

  auto& column_1 = *(_test_table->get_chunk(ChunkID{0})->get_column(CxlumnID{0}));
  auto& column_2 = *(_test_table->get_chunk(ChunkID{1})->get_column(CxlumnID{0}));

  EXPECT_EQ(ref_column[0], column_1[2]);
  EXPECT_TRUE(variant_is_null(ref_column[1]) && variant_is_null(column_2[0]));
  EXPECT_EQ(ref_column[2], column_2[1]);
}

TEST_F(ReferenceSegmentTest, RetrieveNullValueFromNullRowID) {
  // PosList with (0, 0), (0, 1), NULL_ROW_ID, (0, 2)
  auto pos_list = std::make_shared<PosList>(
      std::initializer_list<RowID>({RowID{ChunkID{0u}, ChunkOffset{0u}}, RowID{ChunkID{0u}, ChunkOffset{1u}},
                                    NULL_ROW_ID, RowID{ChunkID{0u}, ChunkOffset{2u}}}));

  auto ref_column = ReferenceSegment(_test_table, CxlumnID{0u}, pos_list);

  auto& column = *(_test_table->get_chunk(ChunkID{0u})->get_column(CxlumnID{0u}));

  EXPECT_EQ(ref_column[0], column[0]);
  EXPECT_EQ(ref_column[1], column[1]);
  EXPECT_TRUE(variant_is_null(ref_column[2]));
  EXPECT_EQ(ref_column[3], column[2]);
}

TEST_F(ReferenceSegmentTest, MemoryUsageEstimation) {
  /**
   * WARNING: Since it's hard to assert what constitutes a correct "estimation", this just tests basic sanity of the
   * memory usage estimations
   */

  const auto pos_list_a = std::make_shared<PosList>();
  pos_list_a->emplace_back(RowID{ChunkID{0}, ChunkOffset{0}});
  pos_list_a->emplace_back(RowID{ChunkID{0}, ChunkOffset{1}});
  const auto pos_list_b = std::make_shared<PosList>();

  ReferenceSegment reference_segment_a(_test_table, CxlumnID{0}, pos_list_a);
  ReferenceSegment reference_segment_b(_test_table, CxlumnID{0}, pos_list_b);

  EXPECT_EQ(reference_segment_a.estimate_memory_usage(), reference_segment_b.estimate_memory_usage() + 2 * sizeof(RowID));
}

}  // namespace opossum