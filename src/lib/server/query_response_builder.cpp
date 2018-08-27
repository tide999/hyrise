#include "query_response_builder.hpp"

#include "server/postgres_wire_handler.hpp"
#include "sql/sql_pipeline.hpp"

#include "SQLParserResult.h"

#include "then_operator.hpp"

namespace opossum {

using opossum::then_operator::then;

std::vector<ColumnDescription> QueryResponseBuilder::build_row_description(const std::shared_ptr<const Table>& table) {
  std::vector<ColumnDescription> result;

  const auto& cxlumn_names = table->cxlumn_names();
  const auto& cxlumn_types = table->cxlumn_data_types();

  for (auto cxlumn_id = 0u; cxlumn_id < table->cxlumn_count(); ++cxlumn_id) {
    uint32_t object_id;
    int32_t type_id;

    switch (cxlumn_types[cxlumn_id]) {
      case DataType::Int:
        object_id = 23;
        type_id = 4;
        break;
      case DataType::Long:
        object_id = 20;
        type_id = 8;
        break;
      case DataType::Float:
        object_id = 700;
        type_id = 4;
        break;
      case DataType::Double:
        object_id = 701;
        type_id = 8;
        break;
      case DataType::String:
        object_id = 25;
        type_id = -1;
        break;
      default:
        Fail("Bad DataType");
    }

    result.emplace_back(ColumnDescription{cxlumn_names[cxlumn_id], object_id, type_id});
  }

  return result;
}

std::string QueryResponseBuilder::build_command_complete_message(hsql::StatementType statement_type,
                                                                 uint64_t row_count) {
  switch (statement_type) {
    case hsql::StatementType::kStmtSelect: {
      return "SELECT " + std::to_string(row_count);
    }
    case hsql::StatementType::kStmtInsert: {
      // 0 is ignored OID and 1 inserted row
      return "INSERT 0 1";
    }
    case hsql::StatementType::kStmtUpdate: {
      // We do not return how many rows are affected, because we don't track this information
      return "UPDATE -1";
    }
    case hsql::StatementType::kStmtDelete: {
      // We do not return how many rows are affected, because we don't track this information
      return "DELETE -1";
    }
    case hsql::StatementType::kStmtCreate: {
      // 0 rows retrieved (Postgres requires a CREATE TABLE statement to return SELECT)
      return "SELECT 0";
    }
    default: { throw std::logic_error("Unknown statement type. Server doesn't know how to complete query."); }
  }
}

std::string QueryResponseBuilder::build_execution_info_message(const std::shared_ptr<SQLPipeline>& sql_pipeline) {
  return sql_pipeline->metrics().to_string();
}

boost::future<uint64_t> QueryResponseBuilder::send_query_response(const send_row_t& send_row, const Table& table) {
  // Essentially we're iterating over every row in every chunk in the table, generating and sending
  // its string representation. However, because of the asynchronous send_row call, we have to
  // use this two-level recursion instead of two nested for-loops

  return _send_query_response_chunks(send_row, table, ChunkID{0}) >> then >> [&]() { return table.row_count(); };
}

boost::future<void> QueryResponseBuilder::_send_query_response_chunks(const send_row_t& send_row, const Table& table,
                                                                      ChunkID current_chunk_id) {
  if (current_chunk_id == table.chunk_count()) return boost::make_ready_future();

  const auto& chunk = table.get_chunk(current_chunk_id);

  return _send_query_response_rows(send_row, *chunk, ChunkOffset{0}) >> then >>
         std::bind(QueryResponseBuilder::_send_query_response_chunks, send_row, std::ref(table),
                   ChunkID{current_chunk_id + 1});
}

boost::future<void> QueryResponseBuilder::_send_query_response_rows(const send_row_t& send_row, const Chunk& chunk,
                                                                    ChunkOffset current_chunk_offset) {
  if (current_chunk_offset == chunk.size()) return boost::make_ready_future();

  std::vector<std::string> row_strings(chunk.cxlumn_count());

  for (CxlumnID cxlumn_id{0}; cxlumn_id < CxlumnID{chunk.cxlumn_count()}; ++cxlumn_id) {
    const auto& column = chunk.get_column(cxlumn_id);
    row_strings[cxlumn_id] = type_cast<std::string>((*column)[current_chunk_offset]);
  }

  return send_row(row_strings) >> then >> std::bind(QueryResponseBuilder::_send_query_response_rows, send_row,
                                                    std::ref(chunk), ChunkOffset{current_chunk_offset + 1});
}

}  // namespace opossum
