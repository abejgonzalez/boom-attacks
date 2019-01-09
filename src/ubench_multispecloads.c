#include <stdio.h>
#include <stdint.h> 
#include "encoding.h"
#include "cache.h"

#define TRAIN_TIMES 6 // assumption is that you have a 2 bit counter in the predictor
#define ROUNDS 1 // run the train + attack sequence X amount of times (for redundancy)

uint8_t array[L1_SZ_BYTES*2];
uint8_t* alignedMem;

// ubench: have multiple spec loads with the same idx
// this should stall on until the load finishes with that idx
// these spec loads should resolve into the dcache
int main(void){
    uint64_t start, diff;
    uint8_t dummy;
    alignedMem = (uint8_t*)((((uint64_t)&array) + L1_SZ_BYTES) & TAG_MASK);

    for(int64_t j = ((TRAIN_TIMES+1)*ROUNDS)-1; j >= 0; --j){

        flushCache((uint64_t)alignedMem, L1_BLOCK_SZ_BYTES);

        // set of constant takens to make the BHR be in a all taken state
        for(uint64_t k = 0; k < 30; ++k){
            asm("");
        }

        // train or attack
        asm("auipc a4, 0x2a\n" // start of seq to get a4 = alignedMem
            "addi a4, a4, -1412\n" //TODO: Figure out offset to array
            "lui a5, 0x8\n"
            "add a4, a4, a5\n"
            "lui a5, 0xfffff\n"
            "and a4, a4, a5\n" // a4 = alignedMem
            "addi a4, a4, -2\n"
            "addi t1, zero, 2\n"
            "slli t2, t1, 0x4\n"
            "fcvt.s.lu fa4, t1\n"
            "fcvt.s.lu fa5, t2\n"
            "fdiv.s	fa5, fa5, fa4\n"
            "fdiv.s	fa5, fa5, fa4\n"
            "fdiv.s	fa5, fa5, fa4\n"
            "fdiv.s	fa5, fa5, fa4\n"
            "fcvt.lu.s	t2, fa5, rtz\n"
            "add a4, a4, t2\n" // a4 = alignedMem --- but very delayed

            "lbu a4, 0(a4)\n" // load tag 0 set 0 addr (a4 = mem[alignedMem])
            "li a5, 0\n" // set a5 to 0
            "nop\n"
            "bne a5, a4, done_label\n" // Should branch to after the last load // TODO: Figure how to branch // 10 instructions * 4 bytes per

            "auipc a4, 0x2a\n" // start of seq to get a5 = alignedMem + 0x1000
            "addi a4, a4, -1496\n" //TODO: Figure out offset to array
            "lui a5, 0x8\n"
            "add a4, a4, a5\n"
            "lui a5, 0xfffff\n"
            "and a4, a4, a5\n" // a4 = alignedMem
            "lui a5, 0x1\n" 
            "add a5, a4, a5\n" // a5 = alignedMem + 0x1000

            "lbu a5, 0(a5)\n" // load tag 1 set 0 addr (a5 = mem[alignedMem + 0x1000])
            "rdcycle a5\n" // bound speculation
            "done_label:\n"
            "nop\n"
            :
            : 
            : "a4", "a5", "t1", "t2", "fa4", "fa5");
    }

    start = rdcycle();

    // access set 0, way 1
    dummy += *((alignedMem + 0x1000));

    diff = rdcycle() - start;

    printf( "Results: Time to run: (%lu cycles)\n", diff);
}
