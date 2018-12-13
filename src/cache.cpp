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

#include "cache.h"
#include "hash.h"

#include "event_recorder.h"
#include "timing_event.h"
#include "zsim.h"

#define _GRAPHETCH 
#undef _GRAPHETCH

#ifdef _GRAPHETCH
#include <map>
#include <vector>
#include <algorithm>

std::vector<uint64_t> MCnodelist;
std::map<uint64_t, uint64_t> MCnodestatus;

#include "graphetch/graphnode.h"

uint64_t lastResp = 0;

#endif
int64_t doneSince = 0;

Cache::Cache(uint32_t _numLines, CC* _cc, CacheArray* _array, ReplPolicy* _rp, uint32_t _accLat, uint32_t _invLat, const g_string& _name)
    : cc(_cc), array(_array), rp(_rp), numLines(_numLines), accLat(_accLat), invLat(_invLat), name(_name) {}

const char* Cache::getName() {
    return name.c_str();
}

void Cache::setParents(uint32_t childId, const g_vector<MemObject*>& parents, Network* network) {
    cc->setParents(childId, parents, network);
}

void Cache::setChildren(const g_vector<BaseCache*>& children, Network* network) {
    cc->setChildren(children, network);
}

void Cache::initStats(AggregateStat* parentStat) {
    AggregateStat* cacheStat = new AggregateStat();
    cacheStat->init(name.c_str(), "Cache stats");
    initCacheStats(cacheStat);
    parentStat->append(cacheStat);
}

void Cache::initCacheStats(AggregateStat* cacheStat) {
    cc->initStats(cacheStat);
    array->initStats(cacheStat);
    rp->initStats(cacheStat);
}

uint64_t Cache::access(MemReq& req) {
    uint64_t respCycle = req.cycle;
    bool skipAccess = cc->startAccess(req); //may need to skip access due to races (NOTE: may change req.type!)
    bool isGraphetch = req.flags & MemReq::GRAPHETCH;

    if (likely(!skipAccess)) {
        bool updateReplacement = (req.type == GETS) || (req.type == GETX);
        int32_t lineId = array->lookup(req.lineAddr, &req, updateReplacement);
        respCycle += accLat;

        if (lineId == -1 && cc->shouldAllocate(req)) {
            //Make space for new line
            Address wbLineAddr;
            lineId = array->preinsert(req.lineAddr, &req, &wbLineAddr); //find the lineId to replace
            trace(Cache, "[%s] Evicting 0x%lx", name.c_str(), wbLineAddr);

            //Evictions are not in the critical path in any sane implementation -- we do not include their delays
            //NOTE: We might be "evicting" an invalid line for all we know. Coherence controllers will know what to do
            if(isGraphetch)
                info("cache: %s: processEviction", name.c_str());
            
            cc->processEviction(req, wbLineAddr, lineId, respCycle); //1. if needed, send invalidates/downgrades to lower level

            array->postinsert(req.lineAddr, &req, lineId); //do the actual insertion. NOTE: Now we must split insert into a 2-phase thing because cc unlocks us.
        }

        // Enforce single-record invariant: Writeback access may have a timing
        // record. If so, read it.
        EventRecorder* evRec = zinfo->eventRecorders[req.srcId];
        TimingRecord wbAcc;
        wbAcc.clear();
        if (unlikely(evRec && evRec->hasRecord())) {
            wbAcc = evRec->popRecord();
        }

#define LLC "l2-0" /* Change this manually for now when graphetch.cfg changes */
#ifdef _GRAPHETCH

#define SIMPLE_MEMORY_LATENCY 100 /* Change manually if graphetch.cfg changes for now... */

        // METHOD 1: Squeeze in requests when there was enough time available.
        // reset respCycle to be equal to req.cycle if Graphetch'd!
        // As we are only hiding latency based on the time between successive requests to memory, 
        // we assume that we fetched the node that is being requested right now making this method
        // optimal in the sense of predicting the future (We knew what would miss from cache).

        /* access will go to main memory */
        if(name == LLC) {
            if((req.type == GETS) || (req.type == GETX)) {

                int64_t diff = req.cycle - lastResp;
                assert(diff >= 0);

                if(diff >= SIMPLE_MEMORY_LATENCY) {
                    int64_t slots = diff / SIMPLE_MEMORY_LATENCY;
                    doneSince += slots; // The if condition is implicit in this line
                    info("memaccplus: in %lu, out %lu", lastResp, lastResp + (slots * SIMPLE_MEMORY_LATENCY));
                }

                if(isGraphetch && (doneSince > 0)) {
                    info("MOFU");
                    doneSince -= 1;
                    respCycle = cc->processAccess(req, lineId, respCycle);
                    respCycle = req.cycle;
                }
                else {
                    respCycle = cc->processAccess(req, lineId, respCycle);
                }

                info("memaccllc: in %lu, out %lu", req.cycle, respCycle);
                lastResp = respCycle;
            }
            else {
                respCycle = cc->processAccess(req, lineId, respCycle);
            }
        }
        else { /* NOT LLC */
            respCycle = cc->processAccess(req, lineId, respCycle);
        }

        //     if(((req.type == GETS) || (req.type == GETX))) {
        //         if(isGraphetch && (doneSince > 0)) {
        //             info("MOFU");
        //             doneSince -= 1;
        //             respCycle = cc->processAccess(req, lineId, respCycle);
        //             respCycle = req.cycle;
        //         }
        //         else {
        //             respCycle = cc->processAccess(req, lineId, respCycle);
        //         }
        //     }
        //     else {
        //         respCycle = cc->processAccess(req, lineId, respCycle);
        //     }

        //     lastResp = respCycle;

        //     info("memaccllc: in %lu, out %lu", lastResp, lastResp + (slots * SIMPLE_MEMORY_LATENCY));
        // }
        // else {
        //     respCycle = cc->processAccess(req, lineId, respCycle);
        // }

        // if((req.type == GETS || req.type == GETX) && (diff >= SIMPLE_MEMORY_LATENCY)) {
        //     int64_t slots = diff / SIMPLE_MEMORY_LATENCY;
        //     doneSince += slots; // The if condition is implicit in this line
        //     info("memaccplus: in %lu, out %lu", lastResp, lastResp + (slots * SIMPLE_MEMORY_LATENCY));
        // }

        // // if(isGraphetch) {
        // //     info("GRAPHETCH at %s", name.c_str());
        // // }

        // if (isGraphetch && (req.type == GETS || req.type == GETX) && (name == LLC)) {
        //     if(doneSince > 0) {
        //         info("MOFU");
        //         doneSince -= 1;
        //         respCycle = cc->processAccess(req, lineId, respCycle);
        //         respCycle = req.cycle;
        //     }
        //     else
        //         respCycle = cc->processAccess(req, lineId, respCycle);
        // }
        // else {
        //     respCycle = cc->processAccess(req, lineId, respCycle);
        //     if (req.type == GETS || req.type == GETX) {
        //         info("memaccg: in %lu, out %lu", req.cycle, respCycle);
        //     }
        // }

        // lastResp = respCycle;
#else
        respCycle = cc->processAccess(req, lineId, respCycle);
        if((req.type == GETS || req.type == GETX) && (name == LLC)) {
            info("memacc: in %lu, out %lu", req.cycle, respCycle);
        }
#endif

        // Access may have generated another timing record. If *both* access
        // and wb have records, stitch them together
        if (unlikely(wbAcc.isValid())) {
            if (!evRec->hasRecord()) {
                // Downstream should not care about endEvent for PUTs
                wbAcc.endEvent = nullptr;
                evRec->pushRecord(wbAcc);
            } else {
                // Connect both events
                //                info("Timing..");

                TimingRecord acc = evRec->popRecord();
                assert(wbAcc.reqCycle >= req.cycle);
                assert(acc.reqCycle >= req.cycle);
                DelayEvent* startEv = new (evRec) DelayEvent(0);
                DelayEvent* dWbEv = new (evRec) DelayEvent(wbAcc.reqCycle - req.cycle);
                DelayEvent* dAccEv = new (evRec) DelayEvent(acc.reqCycle - req.cycle);
                startEv->setMinStartCycle(req.cycle);
                dWbEv->setMinStartCycle(req.cycle);
                dAccEv->setMinStartCycle(req.cycle);
                startEv->addChild(dWbEv, evRec)->addChild(wbAcc.startEvent, evRec);
                startEv->addChild(dAccEv, evRec)->addChild(acc.startEvent, evRec);

                acc.reqCycle = req.cycle;
                acc.startEvent = startEv;
                // endEvent / endCycle stay the same; wbAcc's endEvent not connected
                evRec->pushRecord(acc);
            }
        }
    }

    cc->endAccess(req);

    assert_msg(respCycle >= req.cycle, "[%s] resp < req? 0x%lx type %s childState %s, respCycle %ld reqCycle %ld",
            name.c_str(), req.lineAddr, AccessTypeName(req.type), MESIStateName(*req.state), respCycle, req.cycle);
    return respCycle;
}

void Cache::startInvalidate() {
    cc->startInv(); //note we don't grab tcc; tcc serializes multiple up accesses, down accesses don't see it
}

uint64_t Cache::finishInvalidate(const InvReq& req) {
    int32_t lineId = array->lookup(req.lineAddr, nullptr, false);
    assert_msg(lineId != -1, "[%s] Invalidate on non-existing address 0x%lx type %s lineId %d, reqWriteback %d", name.c_str(), req.lineAddr, InvTypeName(req.type), lineId, *req.writeback);
    uint64_t respCycle = req.cycle + invLat;
    trace(Cache, "[%s] Invalidate start 0x%lx type %s lineId %d, reqWriteback %d", name.c_str(), req.lineAddr, InvTypeName(req.type), lineId, *req.writeback);
    respCycle = cc->processInv(req, lineId, respCycle); //send invalidates or downgrades to children, and adjust our own state
    trace(Cache, "[%s] Invalidate end 0x%lx type %s lineId %d, reqWriteback %d, latency %ld", name.c_str(), req.lineAddr, InvTypeName(req.type), lineId, *req.writeback, respCycle - req.cycle);

    return respCycle;
}
