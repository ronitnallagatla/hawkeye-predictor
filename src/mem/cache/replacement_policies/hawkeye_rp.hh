#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__

#include <unordered_map>
#include <vector>

#include "base/sat_counter.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/optgen.hh"
#include "mem/packet.hh"

namespace gem5
{

struct HawkeyeRPParams;

GEM5_DEPRECATED_NAMESPACE (ReplacementPolicy, replacement_policy);
namespace replacement_policy
{

class Hawkeye : public Base
{
protected:
  struct HawkeyeReplData : ReplacementData
  {
    uint64_t pc;
    uint32_t load_timestamp;
    uint32_t set;
    uint32_t way;

    HawkeyeReplData () : pc (0), load_timestamp (0), set (0), way (0) {}

    // clang-format off
      uint32_t getSet() const { return set; }
      uint32_t getWay() const { return way; }
    // clang-format on
  };

  class Predictor
  {
    uint32_t max_shct;
    uint32_t shct_size;
    std::unordered_map<uint64_t, int8_t> predictor;

  public:
    bool get_prediction (uint64_t pc);
    void train (uint64_t pc);
    void detrain (uint64_t pc);

    Predictor (uint32_t _max_shct, uint32_t _shct_size)
        : max_shct (_max_shct), shct_size (_shct_size)
    {
    }
  };

  mutable Predictor predictor;

  std::vector<std::vector<SatCounter8> > rrpv;
  mutable std::vector<std::vector<bool> > free_ways;

  std::vector<OPTgen> optgen_per_set;

  std::vector<uint32_t> global_timestamp;

  uint32_t num_sets;
  uint32_t num_ways;
  int cache_line_size;
  uint32_t occupancy_vec_size;

  // void update_state(const std::shared_ptr<HawkeyeReplData>&
  // replacement_data,
  //  uint32_t evicted_rrpv, uint32_t evicted_way);

public:
  typedef HawkeyeRPParams Params;
  Hawkeye (const Params &p);
  ~Hawkeye () = default;

  /**
   *
   * @param replacement_data Replacement data to be invalidated.
   */
  void invalidate (
      const std::shared_ptr<ReplacementData> &replacement_data) override;

  /**
   * Touch an entry to update its replacement data.
   *
   * @param replacement_data Replacement data to be touched.
   */

  void touch (const std::shared_ptr<ReplacementData> &replacement_data,
              const PacketPtr pkt) override;
  void touch (
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  /**
   *
   * @param replacement_data Replacement data to be reset.
   */

  void reset (const std::shared_ptr<ReplacementData> &replacement_data,
              const PacketPtr pkt) override;
  void reset (
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  /**
   * Find replacement victim in set using Hawkeye policy.
   *
   * @param candidates Replacement candidates, selected by indexing policy.
   * @return Replacement entry to be replaced.
   */
  ReplaceableEntry *
  getVictim (const ReplacementCandidates &candidates) const override;

  /**
   * Instantiate a replacement data entry.
   *
   * @return A shared pointer to the new replacement data.
   */
  std::shared_ptr<ReplacementData> instantiateEntry () override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__
