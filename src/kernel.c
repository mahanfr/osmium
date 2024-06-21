#include "../common/uefi_data.h"
#include "graphics/basic_graphics.h"
#include <stdint.h>
#include <stdbool.h>
#include "memory.h"
#include "paging/page_frame_allocator.h"
#include "paging/page_map_indexer.h"
#include "paging/page_table_manager.h"
#include "paging/paging.h"

extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;

void _start(UEFIBootData* uefiBootData) {
    (void) initBasicGraphics(uefiBootData);
    print("Hello World From Kernel!\n");

    // Init PageFrame Memory Mapping
    (void) pageFrameInitEfiMemoryMap(
            uefiBootData->mMap,
            uefiBootData->mMapEntries,
            uefiBootData->mMapDescSize);


    // Locking Kernels location in memory
    uint64_t kernelSize = (uint64_t)&_KernelEnd - (uint64_t)&_KernelStart;
    uint64_t kernelPages = (uint64_t)(kernelSize / 4096) + 1;
    pageFrameLockPages(&_KernelStart, kernelPages);
    pageFrameLockPages(uefiBootData->frameBuffer->BaseAddress, uefiBootData->frameBuffer->BufferSize);
    print("Locking FrameBuffer\n");

    // Create a PageTableManager
    PageTable* PML4 = (PageTable*) pageFrameRequestPage();
    memSet(PML4, 0, MEM_FRAME_SIZE);
    PageTableManager* pageTableManager = pageTableManager_Create(PML4);

    // Map All the Pages of Memory to thire Virtual Memory
    uint64_t memorySize = getMemorySize(uefiBootData->mMap, uefiBootData->mMapEntries, uefiBootData->mMapDescSize);
    for(uint64_t i = 0; i < memorySize; i+= MEM_FRAME_SIZE) {
        pageTableManager_MapMemory(pageTableManager, (void*)i, (void*)i);
    }
    print("Table Manager Mapped\n");
    // Map All the Pages of Memory to thire Virtual Memory
    for(uint64_t i = (uint64_t) uefiBootData->frameBuffer->BaseAddress;
            i < (uint64_t)uefiBootData->frameBuffer->BaseAddress + uefiBootData->frameBuffer->BufferSize;
            i+= MEM_FRAME_SIZE) {
        pageTableManager_MapMemory(pageTableManager, (void*)i, (void*)i);
    }
    print("Mapping FrameBuffer\n");

    asm("mov %0, %%cr3" :: "r" (PML4));

    pageTableManager_MapMemory(pageTableManager, (void*)0x600000000, (void*) 0x80000);
    uint64_t* test = (uint64_t*) 0x600000000;
    *test = 26;

    print("Test is: %d\n", *test);

    return;
}
