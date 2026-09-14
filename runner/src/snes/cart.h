
#ifndef CART_H
#define CART_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Cart Cart;
typedef struct SuperFx SuperFx;
typedef struct Cx4 Cx4;
typedef struct Dsp1 Dsp1;
typedef struct Sa1 Sa1;

#include "snes.h"

struct Cart {
  Snes* snes;
  uint8_t type;

  uint8_t* rom;
  uint32_t romSize;
  /* Host-selected expanded HiROM data banks. Cartridge configuration, not
   * mutable guest/save-state data; disabled again by cart_load. */
  bool hiromLinearDataBanks;
  uint8_t* ram;
  uint32_t ramSize;
  const uint64_t* masterClock;
  uint32_t cpuBusAddress;
  SuperFx* superfx;
  Cx4* cx4;
  Dsp1* dsp1;
  Sa1* sa1;
};

enum {
  CART_LOROM = 1,
  CART_HIROM = 2,
  CART_SUPERFX = 3,
  CART_CX4 = 4,
  CART_DSP1 = 5,
  CART_DSP1_HIROM = 6,
  CART_SA1 = 7
};

static inline bool cart_has_sa1(const Cart *cart) {
  return cart && cart->type == CART_SA1 && cart->sa1;
}

static inline bool cart_has_dsp1(const Cart* cart) {
  return cart &&
         (cart->type == CART_DSP1 || cart->type == CART_DSP1_HIROM);
}

/* True when (bank, adr) lands in the Capcom Cx4's CPU-visible window:
 * banks $00-$3F / $80-$BF, address $6000-$7FFF. Callers on the fast bus
 * paths (cpu_state.c) use this to route to cart_read/cart_write instead of
 * falling through to a ROM pointer. */
static inline bool cart_is_cx4_window(const Cart* cart, uint8_t bank,
                                     uint16_t adr) {
  return cart && cart->type == CART_CX4 && adr >= 0x6000 && adr < 0x8000 &&
         (bank < 0x40 || (bank >= 0x80 && bank < 0xc0));
}

/* Super Mario Kart's SHVC-1K1X DSP-1 host port:
 * banks $00-$1F / $80-$9F, addresses $6000-$7FFF, mirrored by mask $0FFF.
 * The mapper removes the low 12 address bits: $6000-$6FFF selects DR and
 * $7000-$7FFF selects SR. */
static inline bool cart_is_dsp1_window(const Cart* cart, uint8_t bank,
                                      uint16_t adr) {
  return cart_has_dsp1(cart) && adr >= 0x6000 && adr < 0x8000 &&
         (bank < 0x20 || (bank >= 0x80 && bank < 0xa0));
}

static inline uint16_t cart_dsp1_register(uint16_t adr) {
  return (uint16_t)((adr >> 12) & 1u);
}

/* SHVC-1K1X battery SRAM is separate from the usual LoROM $70-$7D window. */
static inline bool cart_is_dsp1_sram_window(const Cart* cart, uint8_t bank,
                                            uint16_t adr) {
  return cart_has_dsp1(cart) && adr >= 0x6000 && adr < 0x8000 &&
         ((bank >= 0x20 && bank < 0x40) ||
          (bank >= 0xa0 && bank < 0xc0));
}

void cart_sync_coprocessors(Cart *cart, uint64_t master_clock);
void cart_set_master_clock_source(Cart *cart, const uint64_t *master_clock);
void cart_note_cpu_bus(Cart *cart, uint8_t bank, uint16_t address);

// TODO: how to handle reset & load? (especially where to init ram)

Cart* cart_init(Snes* snes);
void cart_free(Cart* cart);
void cart_reset(Cart* cart); // will reset special chips etc, general reading is set up in load
void cart_load(Cart* cart, int type, uint8_t* rom, int romSize, int ramSize); // TODO: figure out how to handle (battery, cart-chips etc)
/* Opt in only for a verified expanded image: $40-$7D address the additional
 * linear data area; $80-$FF retain the original HiROM mirrors. This is not
 * automatic ExHiROM header detection or a change to stock HiROM. */
static inline void cart_set_hirom_linear_data_banks(Cart *cart, bool enabled) {
  if (cart)
    cart->hiromLinearDataBanks = enabled && cart->type == CART_HIROM &&
                                cart->romSize > 0x400000u;
}

/* Shared by the CPU pointer path and cartridge/DMA bus reads. Keep this
 * address translation separately testable with synthetic cartridge storage. */
static inline uint8_t *cart_hirom_rom_ptr(Cart *cart, uint8_t bank,
                                        uint16_t adr) {
  if (!cart || !cart->rom || !cart->romSize || cart->type != CART_HIROM ||
      bank == 0x7e || bank == 0x7f)
    return NULL;
  if (cart->hiromLinearDataBanks && bank >= 0x40 && bank < 0x7e) {
    uint32_t off = ((uint32_t)bank << 16) | adr;
    return off < cart->romSize ? &cart->rom[off] : NULL;
  }
  uint8_t canonical = bank & 0x7f;
  if (adr < 0x8000 && canonical < 0x40) return NULL;
  uint32_t off = ((uint32_t)(canonical & 0x3f) << 16) | adr;
  return &cart->rom[off % cart->romSize];
}
uint8_t cart_read(Cart* cart, uint8_t bank, uint16_t adr);
void cart_write(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val);
void cart_saveload(Cart *cart, SaveLoadInfo *sli);
// Resolve a CPU-visible address to stable cartridge storage. Standard carts
// return ROM only; SA-1 may also return its I-RAM or BW-RAM backing. Returns
// NULL for I/O and switched-vector bytes.
uint8_t *cart_getRomPtr(Cart *cart, uint8_t bank, uint16_t adr);
#endif
