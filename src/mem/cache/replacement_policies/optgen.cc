#include "mem/cache/replacement_policies/optgen.hh"

#include <cstdint>

namespace gem5 {

GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
namespace replacement_policy {

OPTgen::OPTgen(uint32_t _cache_capacity, int occupancy_vec_size)
    : cache_capacity(_cache_capacity), num_access(0), num_hits(0),
      num_misses(0), occupancy_vec(occupancy_vec_size, 0) {}

void OPTgen::add_cache_access(uint32_t timestamp) {
  num_access++;
  occupancy_vec[timestamp] = 1;
}

bool OPTgen::get_decision(uint32_t curr_timestamp, uint32_t prev_timestamp) {
  bool is_miss = false;

  for (uint32_t i = prev_timestamp; i != curr_timestamp;
       i = (i + 1) % occupancy_vec.size()) {
    if (occupancy_vec[i] >= cache_capacity) {
      is_miss = true;
      num_misses++;
      break;
    }
  }

  if (!is_miss) {

    for (uint32_t i = prev_timestamp; i != curr_timestamp;
         i = (i + 1) % occupancy_vec.size()) {
      occupancy_vec[i]++;
    }
  }

  return is_miss;
}

} // namespace replacement_policy
} // namespace gem5
