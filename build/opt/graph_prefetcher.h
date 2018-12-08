/** $lic$
 * Copyright (C) 2012-2015 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MEMABS_PREFETCHER_H_
#define MEMABS_PREFETCHER_H_

#include <bitset>
#include "bithacks.h"
#include "g_std/g_string.h"
#include "g_std/g_unordered_map.h"
#include "memory_hierarchy.h"
#include "stats.h"
#include "zsim.h"
#include "core.h"
#include "prefetcher.h"
#include <array>
/* Prefetcher models: Basic operation is to interpose between cache levels, issue additional accesses,
 * and keep a small table with delays; when the demand access comes, we do it and account for the
 * latency as when it was first fetched (to avoid hit latencies on partial latency overlaps).
 */

class StreamPrefetchResponseEvent;
class GraphPrefetchResponseEvent;

/* This is basically a souped-up version of the DLP L2 prefetcher in Nehalem: 16 stream buffers,
 * but (a) no up/down distinction, and (b) strided operation based on dominant stride detection
 * to try to subsume as much of the L1 IP/strided prefetcher as possible.
 *
 * FIXME: For now, mostly hardcoded; 64-line entries (4KB w/64-byte lines), fixed granularities, etc.
 * TODO: Adapt to use weave models
 */
class GraphPrefetcher : public BaseCache {
    private:
        struct Entry {
            // Two competing strides; at most one active
            int32_t stride;
            SatCounter<3, 2, 1> conf;

            struct AccessTimes {
                uint64_t startCycle;  // FIXME: Dead for now, we should use it for profiling
                uint64_t respCycle;

                void fill(uint32_t s, uint64_t r) { startCycle = s; respCycle = r; }
            };

            AccessTimes times[64];
            std::bitset<64> valid;
            std::array<StreamPrefetchResponseEvent*, 64> respEvents;

            uint32_t lastPos;
            uint32_t lastLastPos;
            uint32_t lastPrefetchPos;
            uint64_t lastCycle;  // updated on alloc and hit
            uint64_t ts;

            void alloc(uint64_t curCycle) {
                stride = 1;
                lastPos = 0;
                lastLastPos = 0;
                lastPrefetchPos = 0;
                conf.reset();
                valid.reset();
                respEvents.fill(nullptr);
                lastCycle = curCycle;
            }
        };

        g_string name;
        uint64_t timestamp;  // for LRU
        Address* tag;
        //Stream* streams;
        Entry* array;
        uint8_t type;
        std::bitset<1024> streamsUsed;
        uint32_t numStreams;

        Counter profAccesses, profStreamAccesses, profGraphAccesses, profStreamPrefetches, profDoublePrefetches, profPageHits, profPageMisses, profNumStreams, profHits, profShortHits, profStrideSwitches, profLowConfAccs, profGraphPrefetches;

        MemObject* parent;
        BaseCache* child;
        uint32_t childId;
        uint32_t numBuffers;
        uint32_t depth;
        uint32_t degree;
        uint32_t latencyFactorFixed;

        g_unordered_map<uint64_t/*Line Address*/, uint64_t/*Resp Cycle*/> prefetchTracker;
        g_unordered_map<uint64_t/*Line Address*/, GraphPrefetchResponseEvent*> respEvents2;

    public:
        GraphPrefetcher(const g_string& _name, string _type, uint32_t _numBuffers, uint32_t depth, uint32_t degree);
        ~GraphPrefetcher();
        void initStats(AggregateStat* parentStat);
        const char* getName() { return name.c_str();}
        void setParents(uint32_t _childId, const g_vector<MemObject*>& parents, Network* network);
        void setChildren(const g_vector<BaseCache*>& children, Network* network);

        // void setNewAccessStream(TileInfo tileInfo);

        uint64_t access(MemReq& req);
        uint64_t invalidate(const InvReq& req);
        void simulateStreamPrefetchResponse(StreamPrefetchResponseEvent* ev, uint64_t cycle);
        void simulateGraphPrefetchResponse(GraphPrefetchResponseEvent* ev, uint64_t cycle);
};

#endif  // PREFETCHER_H_
