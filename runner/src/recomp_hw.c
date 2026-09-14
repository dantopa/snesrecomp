#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/ppu.h"
#include "snes/dma.h"
#include "snes/snes.h"

// Recomp hardware-register routing — all internal-register reads and
// writes ($4200-$421F) dispatch to the snes_readReg / snes_writeReg
// impl borrowed from snes9x. Per-register timing semantics are
// documented in docs/VIRTUAL_HW_CONTRACT.md; the prior $4212 override
// here was replaced with an h-counter model inside snes_readReg so all
// callers get coherent state instead of a per-call toggle.

extern Snes *g_snes;
extern Dma *g_dma;
extern const char *g_last_recomp_func;

/* Debug tap for SNESRECOMP_DMA_LOG: the AOT function that performed the
 * most recent $420B write (the name encodes its PC24), consumed by
 * ppu_dma_trace.c. */
const char *g_dmatrace_trigger_fn = "";

void recomp_write_internal_reg(uint16 reg, uint8 val) {
  if (reg == 0x420b)
    g_dmatrace_trigger_fn = g_last_recomp_func ? g_last_recomp_func : "";
  snes_writeReg(g_snes, reg, val);
}

uint8 recomp_read_internal_reg(uint16 reg) {
  return snes_readReg(g_snes, reg);
}
