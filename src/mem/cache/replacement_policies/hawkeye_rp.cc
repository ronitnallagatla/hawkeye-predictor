#include "mem/cache/replacement_policies/hawkeye_rp.hh"

#include "base/intmath.hh"
#include "debug/HawkeyeRP.hh"
#include "params/HawkeyeRP.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);

namespace replacement_policy
{

Hawkeye::Hawkeye(const Params &p)
    : Base(p),
      predictor(p.max_shct, p.shct_size),
      num_sets(p.llc_sets),
      num_ways(p.llc_ways),
      cache_line_size(p.cache_line_size),
      occupancy_vec_size(p.occupancy_vec_size)
{
    rrpv.resize(num_sets);
    for (auto &set_counters : rrpv) {
        set_counters.resize(num_ways, SatCounter8(3, 7));
    }

    free_ways.resize(num_sets, std::vector<bool>(num_ways, true));

    optgen_per_set.reserve(num_sets);
    for (uint32_t i = 0; i < num_sets; ++i) {
        optgen_per_set.emplace_back(num_ways, occupancy_vec_size);
    }

    global_timestamp.resize(num_sets, 0);
}

bool
Hawkeye::Predictor::get_prediction(uint64_t pc)
{
    if (predictor.find(pc) != predictor.end() &&
        predictor[pc] < (max_shct + 1) / 2)
        return false;
    return true;
}

void
Hawkeye::Predictor::train(uint64_t pc)
{
    // init
    if (predictor.find(pc) == predictor.end())
        predictor[pc] = (max_shct + 1) / 2;

    // update
    if (predictor[pc] < max_shct)
        predictor[pc]++;
    else
        predictor[pc] = max_shct;
}

void
Hawkeye::Predictor::detrain(uint64_t pc)
{
    // init
    if (predictor.find(pc) == predictor.end())
        predictor[pc] = (max_shct + 1) / 2;

    // update
    if (predictor[pc] != 0)
        predictor[pc]--;
}

void
Hawkeye::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
{
    auto repl_data =
        std::static_pointer_cast<HawkeyeReplData>(replacement_data);
    free_ways[repl_data->getSet()][repl_data->getWay()] = true;
    rrpv[repl_data->getSet()][repl_data->getWay()].saturate();
}

void
Hawkeye::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("Cant train Hawkeye without access information.");
}

void
Hawkeye::touch(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt)
{
    uint32_t prev_timestamp;
    uint32_t curr_timestamp;
    uint32_t index;
    bool opt_hit;
    bool cache_friendly;

    auto casted_replacement_data =
        std::static_pointer_cast<HawkeyeReplData>(replacement_data);

    // std::cout << "TOUCHING Set " << casted_replacement_data->getSet() << "
    // Way " << casted_replacement_data->getWay() << std::endl;

    // Get previous timestamp using replacement data
    prev_timestamp = casted_replacement_data->load_timestamp;

    // Get current timestamp using global timestamp vector at index given by
    // pkt
    index = casted_replacement_data->getSet();
    curr_timestamp = global_timestamp[index];
    global_timestamp[index] =
        (global_timestamp[index] + 1) % occupancy_vec_size;
    casted_replacement_data->load_timestamp = global_timestamp[index];

    optgen_per_set[index].add_cache_access(curr_timestamp);

    // Call get_decision on optvector given the pkt's index occupancy vector
    opt_hit =
        optgen_per_set[index].get_decision(curr_timestamp, prev_timestamp);

    // CHANGE THIS - Assumes predictor[] returns 1 or 0, but eventually
    // we will change this to implement detraining and have it return a number.
    // Maybe make predictor a class
    // cache_friendly =
    // predictor[static_cast<SignatureType>(replacement_data->pc)];
    cache_friendly = predictor.get_prediction(casted_replacement_data->pc);

    // Update RRPV values
    if (!cache_friendly) {
        rrpv[index][casted_replacement_data->getWay()] = SatCounter8(3, 7);
    } else {
        if (!opt_hit) {
            for (uint8_t i = 0; i < num_ways; i++) {
                if (rrpv[index][i] < 6) {
                    rrpv[index][i]++;
                }
            }
        }
        rrpv[index][casted_replacement_data->getWay()] = SatCounter8(3, 0);
    }
}

void
Hawkeye::reset(const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("Cant train Hawkeye without access information.");
}

void
Hawkeye::reset(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt)
{
    // set replacement_data set,way
    uint32_t way;
    uint32_t index;
    uint8_t cache_friendly;

    auto casted_replacement_data =
        std::static_pointer_cast<HawkeyeReplData>(replacement_data);

    index = (pkt->getAddr() >> floorLog2(cache_line_size)) & (num_sets - 1);

    // std::cout << "Set " << casted_replacement_data->getSet() << ":";
    // for (auto i = 0; i < num_ways; i++) {
    //     std::cout << " " << free_ways[index][i];
    // }
    // std::cout << std::endl;

    for (way = 0; way < num_ways; way++) {
        if (free_ways[index][way]) {
            free_ways[index][way] = false;
            break;
        }
    }

    if (way == num_ways) {
        std::cout << "ERROR: No free ways? Assigning way = 0" << std::endl;
        way = 0;
        free_ways[index][way] = false;
    }

    casted_replacement_data->set = index;
    casted_replacement_data->way = way;
    // std::cout << "RESET Assigning Set " << index << " Way " << way <<
    // std::endl;

    optgen_per_set[index].add_cache_access(global_timestamp[index]);
    global_timestamp[index] =
        (global_timestamp[index] + 1) % occupancy_vec_size;
    casted_replacement_data->load_timestamp = global_timestamp[index];

    // optgen_per_set[index].add_cache_access(curr_timestamp);
    optgen_per_set[index].add_cache_access(global_timestamp[index]);

    if (pkt->req->hasPC())
        casted_replacement_data->pc = pkt->req->getPC();
    else
        casted_replacement_data->pc = 0;

    // SAME CHANGE AS ABOVE - Make this an actual value once you start
    // training/detraining cache_friendly =
    // predictor[static_cast<SignatureType>(replacement_data->pc)];
    cache_friendly = predictor.get_prediction(casted_replacement_data->pc);

    if (!cache_friendly) {
        rrpv[index][casted_replacement_data->getWay()] = SatCounter8(3, 7);
    } else {
        for (int i = 0; i < num_ways; i++) {
            if ((uint8_t)rrpv[index][i] < 6) {
                rrpv[index][i]++;
            }
        }
        rrpv[index][casted_replacement_data->getWay()] = SatCounter8(3, 0);
    }
}

ReplaceableEntry *
Hawkeye::getVictim(const ReplacementCandidates &candidates) const
{
    int evicted_rrpv;
    uint32_t evicted_way;

    auto repl_data_init = std::static_pointer_cast<HawkeyeReplData>(
        candidates[0]->replacementData);

    for (evicted_rrpv = 7; evicted_rrpv >= 0; evicted_rrpv--) {
        for (evicted_way = 0; evicted_way < candidates.size(); evicted_way++) {
            auto repl_data = std::static_pointer_cast<HawkeyeReplData>(
                candidates[evicted_way]->replacementData);
            assert(repl_data != nullptr);
            assert(repl_data->getSet() >= 0);
            assert(repl_data->getWay() >= 0);
            assert(repl_data->getSet() < num_sets);
            assert(repl_data->getWay() < num_ways ||
                   !(std::cerr << "Num ways: " << num_ways << "repl_data way: "
                               << repl_data->getWay() << std::endl));
            assert(rrpv.size() == num_sets);
            assert(rrpv[repl_data->getSet()].size() == num_ways);
            if ((int)rrpv[repl_data->getSet()][repl_data->getWay()] ==
                evicted_rrpv) {
                free_ways[repl_data->getSet()][evicted_way] = true;
                rrpv[repl_data->getSet()][evicted_way].saturate();

                // Update predictor
                if (evicted_rrpv == 7) {
                    predictor.train(repl_data->pc);
                } else {
                    predictor.detrain(repl_data->pc);
                }

                // return evcited candidates
                return candidates[evicted_way];
            }
        }
    }

    std::cout << "ERROR: Loop somehow didn't return anything" << std::endl;
    return candidates[0];

    auto repl_data = std::static_pointer_cast<HawkeyeReplData>(
        candidates[evicted_way]->replacementData);

    free_ways[repl_data->getSet()][evicted_way] = true;
    rrpv[repl_data->getSet()][evicted_way].saturate();

    // Update predictor
    if (evicted_rrpv == 7) {
        predictor.train(repl_data->pc);
    } else {
        predictor.detrain(repl_data->pc);
    }

    // return evcited candidates
    return candidates[evicted_way];
}

/*
void
Hawkeye::update_state(const std::shared_ptr<HawkeyeReplData>& replacement_data,
uint32_t evicted_rrpv, uint32_t evicted_way)
{
  free_ways[replacement_data->getSet()][evicted_way] = false;

  // Update predictor
  if (evicted_rrpv == 7) {
    // predictor[candidates[evicted_way]->pc] = 1;
    // CHANGE THIS TO TRAIN INSTEAD OF SET TO 1
    predictor.train(replacement_data->pc);
  } else {
    // CHANGE THIS TO DETRAIN INSTEAD OF SET TO 0
    predictor.detrain(replacement_data->pc);
  }

}
*/

std::shared_ptr<ReplacementData>
Hawkeye::instantiateEntry()
{
    return std::make_shared<HawkeyeReplData>();
}

} // namespace replacement_policy
} // namespace gem5
