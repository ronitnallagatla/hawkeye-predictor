#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_OPTGEN_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_OPTGEN_HH__

#include <cstdint>
#include <vector>

#include "mem/cache/replacement_policies/base.hh"

namespace gem5 {

GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
namespace replacement_policy {

class OPTgen
{
  uint32_t cache_capacity;
  uint32_t num_access;
  uint32_t num_hits;
  uint32_t num_misses;

  std::vector<unsigned int> occupancy_vec;

public:
  OPTgen(uint32_t _cache_size, int occupancy_vec_size);

  void add_cache_access(uint32_t timestamp);
  bool get_decision(uint32_t curr_timestamp, uint32_t prev_timestamp);
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_OPTGEN_HH_
