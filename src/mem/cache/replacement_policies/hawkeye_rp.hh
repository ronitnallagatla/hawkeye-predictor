#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__

#include <unordered_map>
#include <vector>

#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/optgen.hh"
#include "mem/packet.hh"

namespace gem5
{

struct HawkeyeRPParams;

GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
namespace replacement_policy
{

class Hawkeye : public Base
{
  protected:
    struct HawkeyeReplData : ReplacementData
    {
      uint64_t pc;
      uint32_t load_timestamp;

      HawkeyeReplData() : pc(0), load_timestamp(0) {}
    };

    std::vector<std::vector<uint32_t>> rrpv;

    std::unordered_map<uint64_t, int8_t> predictor;

    std::vector<OPTgen> optgen_per_set;

    uint32_t global_timestamp;
    uint16_t hashPC(uint64_t pc) const;

  public:
    typedef HawkeyeRPParams Params;
    Hawkeye(const Params &p);
    ~Hawkeye() = default;

    /**
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                                    override;

    /**
     * Touch an entry to update its replacement data.
     *
     * @param replacement_data Replacement data to be touched.
     */

    void touch(const std::shared_ptr<ReplacementData>& replacement_data,
        const PacketPtr pkt) override;
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     *
     * @param replacement_data Replacement data to be reset.
     */

    void reset(const std::shared_ptr<ReplacementData>& replacement_data,
        const PacketPtr pkt) override;
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find replacement victim in set using Hawkeye policy.
     *
     * @param candidates Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__
