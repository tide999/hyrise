#include <memory>
#include <vector>

#include "../benchmark_basic_fixture.hpp"
#include "benchmark/benchmark.h"
#include "operators/aggregate.hpp"
#include "operators/table_wrapper.hpp"
#include "types.hpp"

namespace opossum {

BENCHMARK_F(BenchmarkBasicFixture, BM_Aggregate)(benchmark::State& state) {
  _clear_cache();

  std::vector<AggregateCxlumnDefinition> aggregates = {{CxlumnID{1} /* "b" */, AggregateFunction::Min}};

  std::vector<CxlumnID> groupby = {CxlumnID{0} /* "a" */};

  auto warm_up = std::make_shared<Aggregate>(_table_wrapper_a, aggregates, groupby);
  warm_up->execute();
  while (state.KeepRunning()) {
    auto aggregate = std::make_shared<Aggregate>(_table_wrapper_a, aggregates, groupby);
    aggregate->execute();
  }
}

}  // namespace opossum
