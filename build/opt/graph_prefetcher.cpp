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

#include "graph_prefetcher.h"
#include "bithacks.h"
#include "event_recorder.h"
#include "timing_event.h"
#include "g_std/g_vector.h"
#include "zsim.h"
#include<map>
#include <list>
//#define DBG(args...) info(args)
#define DBG(args...)
extern std::list<uint64_t> graphNode;
class StreamPrefetchResponseEvent : public TimingEvent {
    private:
        GraphPrefetcher* pf;
    public:

        const uint32_t idx;
        const uint32_t prefetchPos;

    public:
        StreamPrefetchResponseEvent(GraphPrefetcher* _pf, uint32_t _idx, uint32_t _prefetchPos, int32_t domain) :
            TimingEvent(0, 0, domain), pf(_pf), idx(_idx), prefetchPos(_prefetchPos) {}

        void simulate(uint64_t startCycle) {
            pf->simulateStreamPrefetchResponse(this, startCycle);
            done(startCycle);
        }
};

class GraphPrefetchResponseEvent : public TimingEvent {
    private:
        GraphPrefetcher* pf;
    public:

        const uint32_t prefetchPos;
        const uint64_t prefLineAddr;

    public:
        GraphPrefetchResponseEvent(GraphPrefetcher* _pf, uint32_t _prefetchPos, uint64_t _prefLineAddr, int32_t domain) :
            TimingEvent(0, 0, domain), pf(_pf), prefetchPos(_prefetchPos), prefLineAddr(_prefLineAddr) {}

        void simulate(uint64_t startCycle) {
            pf->simulateGraphPrefetchResponse(this, startCycle);
            done(startCycle);
        }
};

    GraphPrefetcher::GraphPrefetcher(const g_string& _name, string _type, uint32_t _numBuffers, uint32_t _depth, uint32_t _degree)
:name(_name), timestamp(0), numBuffers(_numBuffers), depth(_depth), degree(_degree)
{
    tag = gm_calloc<Address>(numBuffers);
    array = gm_calloc<Entry>(numBuffers);
    streamsUsed.reset();
    numStreams = 0;
    if(_type == "Stream")
        type = 0;
    else if(_type == "Graph")
        type = 1;
    else
        type = 2;
}

GraphPrefetcher::~GraphPrefetcher() {
    gm_free(tag);
    gm_free(array);
    //gm_free(streams);
}


void GraphPrefetcher::setParents(uint32_t _childId, const g_vector<MemObject*>& parents, Network* network) {
    childId = _childId;
    if (parents.size() != 1) panic("Must have one parent");
    if (network) panic("Network not handled");
    parent = parents[0];
}

void GraphPrefetcher::setChildren(const g_vector<BaseCache*>& children, Network* network) {
    if (children.size() != 1) panic("Must have one children");
    if (network) panic("Network not handled");
    child = children[0];
}

void GraphPrefetcher::initStats(AggregateStat* parentStat) {
    AggregateStat* s = new AggregateStat();
    s->init(name.c_str(), "Prefetcher stats");
    profAccesses.init("acc", "Accesses"); s->append(&profAccesses);
    profStreamAccesses.init("streamacc", "Stream Training Accesses"); s->append(&profStreamAccesses);
    profGraphAccesses.init("graphacc", "Graph PF requests"); s->append(&profGraphAccesses);
    profStreamPrefetches.init("streampf", "Issued stream prefetches"); s->append(&profStreamPrefetches);
    profGraphPrefetches.init("graphpf", "Issued stream prefetches"); s->append(&profGraphPrefetches);
    profDoublePrefetches.init("dpf", "Issued double prefetches"); s->append(&profDoublePrefetches);
    profPageHits.init("pghit", "Page/entry hit"); s->append(&profPageHits);
    profPageMisses.init("pgmiss", "Page/entry miss"); s->append(&profPageMisses);
    profNumStreams.init("numstreams", "Number of trained streams"); s->append(&profNumStreams);
    profHits.init("hit", "Prefetch buffer hits, short and full"); s->append(&profHits);
    profShortHits.init("shortHit", "Prefetch buffer short hits"); s->append(&profShortHits);
    profStrideSwitches.init("strideSwitches", "Predicted stride switches"); s->append(&profStrideSwitches);
    profLowConfAccs.init("lcAccs", "Low-confidence accesses with no prefetches"); s->append(&profLowConfAccs);
    parentStat->append(s);
}



uint64_t GraphPrefetcher::access(MemReq& req) {
    uint32_t origChildId = req.childId;
    req.childId = childId;

    if (req.type != GETS && req.type != GETX) return parent->access(req); //other reqs ignored, including stores
    profAccesses.inc();

    uint64_t reqCycle = req.cycle;
    uint64_t respCycle = parent->access(req);

    EventRecorder* evRec = zinfo->eventRecorders[req.srcId];
    TimingRecord acc;
    acc.clear();
    if (unlikely(evRec && evRec->hasRecord())) {
        acc = evRec->popRecord();
    }

    // For performance reasons, we don't want to initialize acc and push a
    // TimingRecord down to the core every access.  Many accesses don't result
    // in prefetches or prefetch-hits, and don't generate timing events. This
    // lambda is called only when acc is needed, and can be safely called
    // multiple times.
    //uint32_t streamID = 0;
    auto initAcc2 = [&]() {
        // Massage acc TimingRecord, which may or may not be valid
        if (acc.isValid()) {  // Due to the demand access or a previous prefetch
            if (acc.reqCycle > reqCycle) {
                DelayEvent* dUpEv = new (evRec) DelayEvent(acc.reqCycle - reqCycle);
                dUpEv->setMinStartCycle(reqCycle);
                dUpEv->addChild(acc.startEvent, evRec);
                acc.reqCycle = reqCycle;
                acc.startEvent = dUpEv;
            }
        } else {
            // Create fixed-delay start and end events, so that we can pass a TimingRecord
            // to the core (we always want to pass a TimingRecord if we issue prefetch accesses,
            // so that we account for latencies correctly even with skews)
            DelayEvent* startEv = new (evRec) DelayEvent(respCycle - reqCycle);
            startEv->setMinStartCycle(reqCycle);
            DelayEvent* endEv = new (evRec) DelayEvent(0);
            endEv->setMinStartCycle(respCycle);
            startEv->addChild(endEv, evRec);
            acc = {req.lineAddr, reqCycle, respCycle, req.type, startEv, endEv};
            assert(acc.isValid());
        }
        // Now acc is valid & startEvent is always at reqCycle
    };

    // added the connection code here  --- compare demand access to the previous prefetch event
    uint64_t pos = req.lineAddr;
    if (evRec && (respEvents2.count(pos) != 0)) {
        if (respEvents2[pos]) {
            //info("prefetch hit ? ");
            initAcc2();
            // Link resp with PrefetchResponseEvent
            assert(acc.respCycle <= respCycle);
            assert(acc.endEvent);
            DelayEvent* dDownEv = new (evRec) DelayEvent(respCycle - acc.respCycle);
            dDownEv->setMinStartCycle(acc.respCycle);
            DelayEvent* dEndEv = new (evRec) DelayEvent(0);
            dEndEv->setMinStartCycle(respCycle);

            acc.endEvent->addChild(dDownEv, evRec)->addChild(dEndEv, evRec);
            respEvents2[pos]->addChild(dEndEv, evRec);

            acc.respCycle = respCycle;
            acc.endEvent = dEndEv;
        }
    }


    //auto issuePrefetch2 = [&](uint64_t prefLineAddr) {

    //    //addedPenaltyFixed = zinfo->prefetcherCycles.curLatency;
    //    DBG("issuing prefetch");
    //    MESIState state = I;
    //    MemReq pfReq = {prefLineAddr, req.type, req.childId, &state, reqCycle, req.childLock, state, req.srcId, MemReq::PREFETCH};
    //    DBG("Prefetching Early: %lx ReqCycle: %lu",pfReq.lineAddr,reqCycle);
    //    uint64_t pfRespCycle = parent->access(pfReq);
    //    DBG("Prefetching Early Done: %lx ReqCycle: %lu RespCycle: %lu",pfReq.lineAddr,reqCycle,pfRespCycle);
    //    assert(state == I);  // prefetch access should not give us any permissions


    //    if (evRec) {  // create & connect weave-phase events
    //        DelayEvent* pfStartEv;
    //        GraphPrefetchResponseEvent* pfEndEv = new (evRec) GraphPrefetchResponseEvent(this, prefLineAddr & (64-1), prefLineAddr, 0 /*FIXME: assign domain @ init*/);
    //        pfEndEv->setMinStartCycle(pfRespCycle);

    //        if (evRec->hasRecord()) {
    //            TimingRecord pfAcc = evRec->popRecord();
    //            assert(pfAcc.isValid());
    //            assert(pfAcc.reqCycle >= reqCycle);
    //            assert(pfAcc.respCycle <= pfRespCycle);
    //            pfStartEv = new (evRec) DelayEvent(pfAcc.reqCycle - reqCycle);
    //            pfStartEv->setMinStartCycle(reqCycle);
    //            pfStartEv->addChild(pfAcc.startEvent, evRec);

    //            DelayEvent* pfDownEv = new (evRec) DelayEvent(pfRespCycle - pfAcc.respCycle);
    //            pfDownEv->setMinStartCycle(pfAcc.respCycle);
    //            pfAcc.endEvent->addChild(pfDownEv, evRec)->addChild(pfEndEv, evRec);
    //        } else {
    //            pfStartEv = new (evRec) DelayEvent(pfRespCycle - reqCycle);
    //            pfStartEv->setMinStartCycle(reqCycle);
    //            pfStartEv->addChild(pfEndEv, evRec);
    //        }

    //        initAcc2();
    //        assert(acc.isValid() && acc.reqCycle == reqCycle);

    //        // Connect prefetch to start event
    //        DelayEvent* startEv = new (evRec) DelayEvent(0);
    //        startEv->setMinStartCycle(reqCycle);
    //        startEv->addChild(acc.startEvent, evRec);
    //        startEv->addChild(pfStartEv, evRec);
    //        acc.startEvent = startEv;
    //        assert(acc.reqCycle == reqCycle);

    //        // Record the PrefetchEndEvent so that we can later connect it with prefetch-hit requests
    //        respEvents2.insert(std::pair<uint64_t, GraphPrefetchResponseEvent*>(prefLineAddr,pfEndEv));
    //    }
    //};
		// Issue prefetcher 2 ends here

		// Code comes here for node checking 
		// uint64_t pos = req.lineAddr;
		std::list<uint64_t>::const_iterator nodeIterator = graphNode.begin();
		for (; nodeIterator != graphNode.end(); ++nodeIterator) {
			info("[INTHISFILE] %lu, %lu", *nodeIterator, pos)
			if (*nodeIterator == pos) {
				info("Found node");
				if (*(int*)(*nodeIterator) == 1) {
					info("Found leaf node");
				}
			}
		}


		bool found_tile = false;
		// What to do of this code? 
    if(found_tile == false){
        if (type == 2) {
        }
        else {
            //info("Tile not found... falling to default");
            profStreamAccesses.inc();
            auto initAcc = [&]() {
                // Massage acc TimingRecord, which may or may not be valid
                if (acc.isValid()) {  // Due to the demand access or a previous prefetch
                    if (acc.reqCycle > reqCycle) {
                        DelayEvent* dUpEv = new (evRec) DelayEvent(acc.reqCycle - reqCycle);
                        dUpEv->setMinStartCycle(reqCycle);
                        dUpEv->addChild(acc.startEvent, evRec);
                        acc.reqCycle = reqCycle;
                        acc.startEvent = dUpEv;
                    }
                } else {
                    // Create fixed-delay start and end events, so that we can pass a TimingRecord
                    // to the core (we always want to pass a TimingRecord if we issue prefetch accesses,
                    // so that we account for latencies correctly even with skews)
                    DelayEvent* startEv = new (evRec) DelayEvent(respCycle - reqCycle);
                    startEv->setMinStartCycle(reqCycle);
                    DelayEvent* endEv = new (evRec) DelayEvent(0);
                    endEv->setMinStartCycle(respCycle);
                    startEv->addChild(endEv, evRec);
                    acc = {req.lineAddr, reqCycle, respCycle, req.type, startEv, endEv};
                    assert(acc.isValid());
                }
                // Now acc is valid & startEvent is always at reqCycle
            };

            Address pageAddr = req.lineAddr >> 6;
            uint32_t pos = req.lineAddr & (64-1);
            uint32_t idx = numBuffers;
            uint32_t startEntry, endEntry;

            startEntry = 0;
            endEntry = numBuffers;
            // This loop gets unrolled and there are no control dependences. Way faster than a break (but should watch for the avoidable loop-carried dep)
            for (uint32_t i = startEntry; i < endEntry; i++) {
                bool match = (pageAddr == tag[i]);
                idx = match?  i : idx;  // ccmov, no branch
            }

            DBG("%s: 0x%lx page %lx pos %d", name.c_str(), req.lineAddr, pageAddr, pos);

            if (idx == numBuffers) {  // entry miss
                profPageMisses.inc();
                uint32_t cand = numBuffers;
                uint64_t candScore = -1;
                //uint64_t candScore = 0;
                for (uint32_t i = 0; i < numBuffers; i++) {
                    DBG("Idx: %u Last cycle: %lu, reqCycle: %lu", i, array[i].lastCycle, reqCycle )
                        if (array[i].lastCycle > reqCycle + 5000) continue;  // warm prefetches, not even a candidate
                    if ((array[i].lastCycle > reqCycle - 5000) && array[i].conf.pred()) continue;  // ensure we're not thrashing
                    if (array[i].ts < candScore) {  // just LRU
                        cand = i;
                        candScore = array[i].ts;
                    }
                }

                if (cand < numBuffers) {
                    idx = cand;
                    array[idx].alloc(reqCycle);
                    array[idx].lastPos = pos;
                    array[idx].ts = timestamp++;
                    tag[idx] = pageAddr;
                }
                DBG("%s: MISS alloc idx %d", name.c_str(), idx);
            } else {  // entry hit
                profPageHits.inc();
                Entry& e = array[idx];
                array[idx].ts = timestamp++;
                DBG("%s: PAGE HIT idx %d", name.c_str(), idx);

                // 1. Did we prefetch-hit?
                bool shortPrefetch = false;
                if (e.valid[pos]) {
                    uint64_t pfRespCycle = e.times[pos].respCycle;
                    shortPrefetch = pfRespCycle > respCycle;
                    e.valid[pos] = false;  // close, will help with long-lived transactions
                    respCycle = MAX(pfRespCycle, respCycle);
                    e.lastCycle = MAX(respCycle, e.lastCycle);
                    DBG("Reqcycle: %lu Setting  lastCycle  of idx: %u to: %lu",reqCycle, idx, e.lastCycle);
                    profHits.inc();
                    if (shortPrefetch) profShortHits.inc();
                    DBG("%s: Addr %lx prefetched on %ld, pf resp %ld, demand resp %ld, short %d", name.c_str(), req.lineAddr, e.times[pos].startCycle, pfRespCycle, respCycle, shortPrefetch);
                    if (evRec && e.respEvents[pos]) {
                        initAcc();
                        // Link resp with PrefetchResponseEvent
                        assert(acc.respCycle <= respCycle);
                        assert(acc.endEvent);
                        DelayEvent* dDownEv = new (evRec) DelayEvent(respCycle - acc.respCycle);
                        dDownEv->setMinStartCycle(acc.respCycle);
                        DelayEvent* dEndEv = new (evRec) DelayEvent(0);
                        dEndEv->setMinStartCycle(respCycle);

                        acc.endEvent->addChild(dDownEv, evRec)->addChild(dEndEv, evRec);
                        e.respEvents[pos]->addChild(dEndEv, evRec);

                        acc.respCycle = respCycle;
                        acc.endEvent = dEndEv;
                    }
                }

                // 2. Update predictors, issue prefetches
                //int32_t stride = pos - e.lastPos;
                //DBG("%s: pos %d lastPos %d lastLastPost %d e.stride %d", name.c_str(), pos, e.lastPos, e.lastLastPos, e.stride);

                //if (e.stride == stride) {
                //    e.conf.inc();
                //    if (e.conf.pred()) {  // do prefetches
                //        int32_t fetchDepth = (e.lastPrefetchPos - e.lastPos)/stride;
                //        uint32_t prefetchPos = e.lastPrefetchPos + stride;
                //        if (fetchDepth < (int32_t)depth)
                //            prefetchPos = e.lastPos + stride*depth;
                //        if (fetchDepth < 1) {
                //            prefetchPos = pos + stride;
                //            fetchDepth = 1;
                //        }
                //        DBG("%s: pos %d stride %d conf %d lastPrefetchPos %d prefetchPos %d fetchDepth %d", name.c_str(), pos, stride, e.conf.counter(), e.lastPrefetchPos, prefetchPos, fetchDepth);

                //        auto issuePrefetch = [&](uint32_t prefetchPos) {
                //            if(!streamsUsed[idx]) {
                //                profNumStreams.inc();
                //                streamsUsed[idx] = true;
                //            }	
                //            DBG("issuing prefetch");
                //            MESIState state = I;
                //            MemReq pfReq = {req.lineAddr + prefetchPos - pos, req.type, req.childId, &state, reqCycle, req.childLock, state, req.srcId, MemReq::PREFETCH};
                //            DBG("Prefetching Early: %lx ReqCycle: %lu",pfReq.lineAddr,reqCycle);
                //            uint64_t pfRespCycle = parent->access(pfReq);
                //            DBG("Prefetching Early Done: %lx ReqCycle: %lu RespCycle: %lu",pfReq.lineAddr,reqCycle,pfRespCycle);

                //            assert(state == I);  // prefetch access should not give us any permissions

                //            e.valid[prefetchPos] = true;
                //            e.times[prefetchPos].fill(reqCycle, pfRespCycle);

                //            if (evRec) {  // create & connect weave-phase events
                //                DelayEvent* pfStartEv;
                //                StreamPrefetchResponseEvent* pfEndEv = new (evRec) StreamPrefetchResponseEvent(this, idx, prefetchPos, 0 /*FIXME: assign domain @ init*/);
                //                pfEndEv->setMinStartCycle(pfRespCycle);

                //                if (evRec->hasRecord()) {
                //                    TimingRecord pfAcc = evRec->popRecord();
                //                    assert(pfAcc.isValid());
                //                    assert(pfAcc.reqCycle >= reqCycle);
                //                    assert(pfAcc.respCycle <= pfRespCycle);
                //                    pfStartEv = new (evRec) DelayEvent(pfAcc.reqCycle - reqCycle);
                //                    pfStartEv->setMinStartCycle(reqCycle);
                //                    pfStartEv->addChild(pfAcc.startEvent, evRec);

                //                    DelayEvent* pfDownEv = new (evRec) DelayEvent(pfRespCycle - pfAcc.respCycle);
                //                    pfDownEv->setMinStartCycle(pfAcc.respCycle);
                //                    pfAcc.endEvent->addChild(pfDownEv, evRec)->addChild(pfEndEv, evRec);
                //                } else {
                //                    pfStartEv = new (evRec) DelayEvent(pfRespCycle - reqCycle);
                //                    pfStartEv->setMinStartCycle(reqCycle);
                //                    pfStartEv->addChild(pfEndEv, evRec);
                //                }

                //                initAcc();
                //                assert(acc.isValid() && acc.reqCycle == reqCycle);

                //                // Connect prefetch to start event
                //                DelayEvent* startEv = new (evRec) DelayEvent(0);
                //                startEv->setMinStartCycle(reqCycle);
                //                startEv->addChild(acc.startEvent, evRec);
                //                startEv->addChild(pfStartEv, evRec);
                //                acc.startEvent = startEv;
                //                assert(acc.reqCycle == reqCycle);

                //                // Record the PrefetchEndEvent so that we can later connect it with prefetch-hit requests
                //                e.respEvents[prefetchPos] = pfEndEv;
                //            }
                //        };
                //        uint32_t num_issued = 0;
                //        if (prefetchPos < 64 /*&& !e.valid[prefetchPos]*/) {
                //            issuePrefetch(prefetchPos);
                //            profStreamPrefetches.inc();
                //            num_issued++;
                //            while ( prefetchPos + stride < 64 /*&& !e.valid[prefetchPos + stride]*/ && num_issued < degree) {
                //                prefetchPos += stride;
                //                issuePrefetch(prefetchPos);
                //                profStreamPrefetches.inc();
                //                num_issued++;
                //                profDoublePrefetches.inc();
                //            }
                //            e.lastPrefetchPos = prefetchPos;
                //        }
                //    } else {
                //        profLowConfAccs.inc();
                //    }
                //} else {
                //    e.conf.dec();
                //    // See if we need to switch strides
                //    if (!e.conf.pred()) {
                //        int32_t lastStride = e.lastPos - e.lastLastPos;

                //        if (stride && stride != e.stride && stride == lastStride) {
                //            e.conf.reset();
                //            e.stride = stride;
                //            profStrideSwitches.inc();
                //        }
                //    }
                //    e.lastPrefetchPos = pos;
                //}

                e.lastLastPos = e.lastPos;
                e.lastPos = pos;
            }

        }
    }

    // Determine prefetched request and issue prefetch	


    if (acc.isValid()) evRec->pushRecord(acc);

    req.childId = origChildId;
    return respCycle;
}

// nop for now; do we need to invalidate our own state?
uint64_t GraphPrefetcher::invalidate(const InvReq& req) {
    return child->invalidate(req);
}


void GraphPrefetcher::simulateGraphPrefetchResponse(GraphPrefetchResponseEvent* ev, uint64_t cycle) {
    // Self-clean so future requests don't get linked to a stale event
    //DBG("[%s] PrefetchResponse %d/%d cycle %ld min %ld", getName(), ev->idx, ev->prefetchPos, cycle, ev->getMinStartCycle());
    //assert(ev->idx < 16 && ev-> prefetchPos < 64);
    if(respEvents2.count(ev->prefLineAddr) != 0){
        auto& evPtr = respEvents2[ev->prefLineAddr];
        // Guard avoids nullifying a pointer that changed before resp arrival (e.g., if entry was reused); this should be rare
        if (evPtr == ev) {
            respEvents2.erase(ev->prefLineAddr);
        } else {
            DBG("[%s] PrefetchResponse already changed (%p)", name.c_str(), evPtr);
        }
    }
}

void GraphPrefetcher::simulateStreamPrefetchResponse(StreamPrefetchResponseEvent* ev, uint64_t cycle) {
    // Self-clean so future requests don't get linked to a stale event
    //DBG("[%s] PrefetchResponse %d/%d cycle %ld min %ld", getName(), ev->idx, ev->prefetchPos, cycle, ev->getMinStartCycle());
    //assert(ev->idx < 16 && ev-> prefetchPos < 64);
    auto& evPtr = array[ev->idx].respEvents[ev->prefetchPos];
    // Guard avoids nullifying a pointer that changed before resp arrival (e.g., if entry was reused); this should be rare
    if (evPtr == ev) {
        evPtr = nullptr;
    } else {
        DBG("[%s] PrefetchResponse already changed (%p)", name.c_str(), evPtr);
    }
}

