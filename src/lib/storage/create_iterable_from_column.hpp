#pragma once

#include "storage/segment_iterables/any_segment_iterable.hpp"
#include "storage/dictionary_segment/dictionary_segment_iterable.hpp"
#include "storage/encoding_type.hpp"

#include "storage/frame_of_reference/frame_of_reference_iterable.hpp"
#include "storage/reference_segment.hpp"
#include "storage/run_length_segment/run_length_segment_iterable.hpp"
#include "storage/value_segment/value_segment_iterable.hpp"

namespace opossum {

template <typename T>
class ReferenceSegmentIterable;

/**
 * @defgroup Uniform interface to create an iterable from a column
 *
 * These methods cannot be part of the columns’ interfaces because
 * reference segment are not templated and thus don’t know their type.
 *
 * All iterables implement the same interface using static polymorphism
 * (i.e. the CRTP pattern, see segment_iterables/.hpp).
 *
 * In debug mode, create_iterable_from_segment returns a type erased
 * iterable, i.e., all iterators have the same type
 *
 * @{
 */

template <typename T>
auto create_iterable_from_segment(const ValueSegment<T>& column) {
  return erase_type_from_iterable_if_debug(ValueSegmentIterable<T>{column});
}

template <typename T>
auto create_iterable_from_segment(const DictionarySegment<T>& column) {
  return erase_type_from_iterable_if_debug(DictionarySegmentIterable<T, pmr_vector<T>>{column});
}

template <typename T>
auto create_iterable_from_segment(const RunLengthSegment<T>& column) {
  return erase_type_from_iterable_if_debug(RunLengthSegmentIterable<T>{column});
}

template <typename T>
auto create_iterable_from_segment(const FixedStringDictionarySegment<T>& column) {
  return erase_type_from_iterable_if_debug(DictionarySegmentIterable<T, FixedStringVector>{column});
}

template <typename T>
auto create_iterable_from_segment(const FrameOfReferenceSegment<T>& column) {
  return erase_type_from_iterable_if_debug(FrameOfReferenceIterable<T>{column});
}

/**
 * This function must be forward-declared because ReferenceSegmentIterable
 * includes this file leading to a circular dependency
 */
template <typename T>
auto create_iterable_from_segment(const ReferenceSegment& column);

/**@}*/

}  // namespace opossum

#include "create_iterable_from_segment.ipp"
