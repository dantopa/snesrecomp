
#ifndef DMA_H
#define DMA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Dma Dma;

typedef void (*DmaVramNotifyHook)(uint8_t aBank, uint16_t aAdr,
                                  uint16_t vmadd, uint16_t size);
void dma_set_vram_notify_hook(DmaVramNotifyHook hook);

#include "snes.h"

typedef struct DmaChannel {
  uint8_t bAdr;
  uint16_t aAdr;
  uint8_t aBank;
  uint16_t size; // also indirect hdma adr
  uint8_t indBank; // hdma
  uint16_t tableAdr; // hdma
  uint8_t repCount; // hdma
  uint8_t unusedByte;
  bool dmaActive;
  bool hdmaActive;
  uint8_t mode;
  bool fixed;
  bool decrement;
  bool indirect; // hdma
  bool fromB;
  bool unusedBit;
  bool doTransfer; // hdma
  bool terminated; // hdma
  uint8_t offIndex;
} DmaChannel;

struct Dma {
  Snes* snes;
  DmaChannel channel[8];
  uint32_t dmaTimer;
  bool dmaBusy;
};

Dma* dma_init(Snes* snes);
void dma_free(Dma* dma);
void dma_reset(Dma* dma);
uint8_t dma_read(Dma* dma, uint16_t adr); // 43x0-43xf
void dma_write(Dma* dma, uint16_t adr, uint8_t val); // 43x0-43xf
void dma_doDma(Dma* dma);
bool dma_cycle(Dma* dma);
void dma_startDma(Dma* dma, uint8_t val, bool hdma);

/* When enabled, a general DMA to $2118 with a raw size of 0 (i.e. 65536
 * bytes) from a cartridge bank other than $80 is treated as a no-op instead
 * of stomping VRAM. The Dixie Kong Country mod's sprite-DMA queue emits
 * zero-size entries that the hack author's emulator silently dropped; stock
 * DKC1's only legitimate zero-size VRAM DMAs are the boot-bank ($80)
 * uploads, which keep working. Default off; only the DKC1 Dixie variant
 * host enables this. */
void dma_set_zero_size_vram_noop(int enabled);
void dma_saveload(Dma *dma, SaveLoadInfo *sli);

#endif
