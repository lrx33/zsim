#ifndef __ZSIM_HOOKS_H__
#define __ZSIM_HOOKS_H__

#include <stdint.h>
#include <stdio.h>

//Avoid optimizing compilers moving code around this barrier
#define COMPILER_BARRIER() { __asm__ __volatile__("" ::: "memory");}

//These need to be in sync with the simulator
#define ZSIM_MAGIC_OP_ROI_BEGIN         (1025)
#define ZSIM_MAGIC_OP_ROI_END           (1026)
#define ZSIM_MAGIC_OP_REGISTER_THREAD   (1027)
#define ZSIM_MAGIC_OP_HEARTBEAT         (1028)
#define ZSIM_MAGIC_OP_WORK_BEGIN        (1029) //ubik
#define ZSIM_MAGIC_OP_WORK_END          (1030) //ubik
#define ZSIM_MAGIC_OP_CACHE_TILE_START  (1031)
#define ZSIM_MAGIC_OP_CACHE_TILE_END    (1032)
#define ZSIM_MAGIC_OP_GRAPH					    (1033)

#ifdef __x86_64__
#define HOOKS_STR  "HOOKS"
static inline void zsim_magic_op(uint64_t op) {
    COMPILER_BARRIER();
    __asm__ __volatile__("xchg %%rcx, %%rcx;" : : "c"(op));
    COMPILER_BARRIER();
}
//static inline void zsim_magic_op_cache (uint64_t op, uint64_t address, uint64_t size) {
//    COMPILER_BARRIER();
//    __asm__ __volatile__("xchg %%rdx, %%rdx;" : : "d"(address));
//    __asm__ __volatile__("xchg %%rdi, %%rdi;" : : "D"(size));
//    __asm__ __volatile__("xchg %%rbx, %%rbx;" : : "b"(op));
//    COMPILER_BARRIER();
//}
static inline void zsim_magic_op_graph (uint64_t op, uint64_t address) {
    printf("Cache Tile Begin with address %lu \n", address);
    COMPILER_BARRIER();
    __asm__ __volatile__("xchg %%rdi, %%rdi;" : : "D"(address));
    __asm__ __volatile__("xchg %%rbx, %%rbx;" : : "b"(op));
    COMPILER_BARRIER();
}
#else
#define HOOKS_STR  "NOP-HOOKS"
static inline void zsim_magic_op(uint64_t op, uint64_t op2) {
    //NOP
}
static inline void zsim_magic_op_cache (uint64_t op, uint64_t address, uint64_t size) {
    //NOP
}
#endif

static void zsim_graph_node(uint64_t address) {
		printf("[" HOOKS_STR "] GRAPH HOOK ");
//    printf("Cache Tile Begin with address %lx \n", address);
//    info("Cache Tile Begin with address %lu \n", address);
    zsim_magic_op_graph(ZSIM_MAGIC_OP_GRAPH, address);
}
//static inline void zsim_cache_tile_begin_1(uint64_t address, uint64_t tile_width) {
//    //printf("Cache Tile Begin with address %lu \n", address);
//    zsim_magic_op_cache(ZSIM_MAGIC_OP_CACHE_TILE_START, address, tile_width);
//}
//static inline void zsim_cache_tile_begin_2(uint64_t total_width, uint64_t tile_height) {
//    //printf("Cache Tile Begin with address %lu \n", address);
//    zsim_magic_op_cache(ZSIM_MAGIC_OP_CACHE_TILE_START, total_width, tile_height);
//}
//static inline void zsim_cache_tile_begin_3(uint64_t tile_num, uint64_t priority) {
//    //printf("Cache Tile Begin with address %lu \n", address);
//    zsim_magic_op_cache(ZSIM_MAGIC_OP_CACHE_TILE_START, tile_num, priority);
//}
//static inline void zsim_cache_tile_begin_4(uint64_t pattern, uint64_t stride) {
//    //printf("Cache Tile Begin with address %lu \n", address);
//    zsim_magic_op_cache(ZSIM_MAGIC_OP_CACHE_TILE_START, pattern, stride);
//}
//static inline void zsim_cache_tile_end() {
//    //printf("Cache Tile End\n");
//    zsim_magic_op_cache(ZSIM_MAGIC_OP_CACHE_TILE_END, 0, 0);
//}
static inline void zsim_roi_begin() {
    printf("[" HOOKS_STR "] ROI begin\n");
    zsim_magic_op(ZSIM_MAGIC_OP_ROI_BEGIN);
}

static inline void zsim_roi_end() {
    zsim_magic_op(ZSIM_MAGIC_OP_ROI_END);
    printf("[" HOOKS_STR  "] ROI end\n");
}

static inline void zsim_heartbeat() {
    zsim_magic_op(ZSIM_MAGIC_OP_HEARTBEAT);
}

static inline void zsim_work_begin() { zsim_magic_op(ZSIM_MAGIC_OP_WORK_BEGIN); }
static inline void zsim_work_end() { zsim_magic_op(ZSIM_MAGIC_OP_WORK_END); }

#endif /*__ZSIM_HOOKS_H__*/
