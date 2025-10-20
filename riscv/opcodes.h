#include "arith.h"
#include "encoding.h"

#include <cassert>
#include <cstdint>

#define ZERO 0
#define T0 5
#define S0 8
#define S1 9

static std::uint32_t
bits(std::uint32_t value, std::uint32_t hi, std::uint32_t lo) {
  assert(hi < 32);
  assert(lo < 32);
  assert(hi >= lo);
  return extract64(value, lo, hi - lo + 1);
}

static std::uint32_t bit(std::uint32_t value, std::uint32_t b) {
  assert(b < 32);
  return extract64(value, b, 1);
}

//================================= J-type =================================//

static std::uint32_t
j_type(std::uint32_t op, std::uint32_t rd, std::uint32_t imm) {
  assert(op < (1u << 7));    // opcode is 7 bits
  assert(rd < (1u << 5));    // rd is 5 bits (register 0-31)
  assert(imm < (1u << 21));  // J-type immediate is 21 bits
  assert((imm & 1) == 0);    // J-type immediate must be 2-byte aligned
  return (bit(imm, 20) << 31) | (bits(imm, 10, 1) << 21) |
         (bit(imm, 11) << 20) | (bits(imm, 19, 12) << 12) | (rd << 7) | op;
}

static std::uint32_t jal(std::uint32_t rd, std::uint32_t imm) {
  return j_type(MATCH_JAL, rd, imm);
}

//================================= R-type =================================//

static std::uint32_t r_type(std::uint32_t op,
                            std::uint32_t rd,
                            std::uint32_t rs1,
                            std::uint32_t rs2) {
  assert(op < (1u << 7));   // opcode is 7 bits
  assert(rd < (1u << 5));   // rd is 5 bits (register 0-31)
  assert(rs1 < (1u << 5));  // rs1 is 5 bits (register 0-31)
  assert(rs2 < (1u << 5));  // rs2 is 5 bits (register 0-31)
  return (rs2 << 20) | (rs1 << 15) | (rd << 7) | op;
}

//================================= S-type =================================//

static std::uint32_t s_type(std::uint32_t op,
                            std::uint32_t rs1,
                            std::uint32_t rs2,
                            std::uint32_t imm) {
  assert(op < (1u << 7));    // opcode is 7 bits
  assert(rs1 < (1u << 5));   // rs1 is 5 bits (register 0-31)
  assert(rs2 < (1u << 5));   // rs2 is 5 bits (register 0-31)
  assert(imm < (1u << 12));  // S-type immediate is 12 bits
  return (bits(imm, 11, 5) << 25) | (rs2 << 20) | (rs1 << 15) |
         (bits(imm, 4, 0) << 7) | op;
}

// Define all S-type store instructions using X-macro
#define STORE_INSNS_LIST(INSN_MACRO) \
  INSN_MACRO(sw,  SW)  \
  INSN_MACRO(sd,  SD)  \
  INSN_MACRO(sh,  SH)  \
  INSN_MACRO(sb,  SB)  \
  INSN_MACRO(fsw, FSW) \
  INSN_MACRO(fsd, FSD)

#define DEFINE_STORE_INSN(name, match)                                  \
  static std::uint32_t name(std::uint32_t rs2,                          \
                            std::uint32_t rs1,                           \
                            std::uint16_t offset) {                      \
    return s_type(MATCH_##match, rs1, rs2, offset);                     \
  }

STORE_INSNS_LIST(DEFINE_STORE_INSN)
#undef DEFINE_STORE_INSN
#undef STORE_INSNS_LIST

//================================= I-type =================================//

static std::uint32_t i_type(std::uint32_t op,
                            std::uint32_t rd,
                            std::uint32_t rs1,
                            std::uint32_t imm) {
  assert(op < (1u << 7));    // opcode is 7 bits
  assert(rd < (1u << 5));    // rd is 5 bits (register 0-31)
  assert(rs1 < (1u << 5));   // rs1 is 5 bits (register 0-31)
  assert(imm < (1u << 12));  // I-type immediate is 12 bits
  return (bits(imm, 11, 0) << 20) | (rs1 << 15) | (rd << 7) | op;
}

// Define all I-type load instructions using X-macro
#define LOAD_INSNS_LIST(INSN_MACRO) \
  INSN_MACRO(ld,  LD)  \
  INSN_MACRO(lw,  LW)  \
  INSN_MACRO(lh,  LH)  \
  INSN_MACRO(lb,  LB)  \
  INSN_MACRO(flw, FLW) \
  INSN_MACRO(fld, FLD)

#define DEFINE_LOAD_INSN(name, match)                                   \
  static std::uint32_t name(std::uint32_t rd,                           \
                            std::uint32_t rs1,                           \
                            std::uint16_t offset) {                      \
    return i_type(MATCH_##match, rd, rs1, offset);                      \
  }

LOAD_INSNS_LIST(DEFINE_LOAD_INSN)
#undef DEFINE_LOAD_INSN
#undef LOAD_INSNS_LIST

// Define all I-type immediate ALU instructions using X-macro
#define IMM_ALU_INSNS_LIST(INSN_MACRO) \
  INSN_MACRO(addi, ADDI) \
  INSN_MACRO(andi, ANDI) \
  INSN_MACRO(ori,  ORI)  \
  INSN_MACRO(xori, XORI)

#define DEFINE_IMM_ALU_INSN(name, match)                                \
  static std::uint32_t name(std::uint32_t rd,                           \
                            std::uint32_t rs1,                           \
                            std::uint16_t imm) {                         \
    return i_type(MATCH_##match, rd, rs1, imm);                         \
  }

IMM_ALU_INSNS_LIST(DEFINE_IMM_ALU_INSN)
#undef DEFINE_IMM_ALU_INSN
#undef IMM_ALU_INSNS_LIST

// Define I-type shift instructions using X-macro
#define SHIFT_INSNS_LIST(INSN_MACRO) \
  INSN_MACRO(slli, SLLI) \
  INSN_MACRO(srli, SRLI)

#define DEFINE_SHIFT_INSN(name, match)                                  \
  static std::uint32_t name(std::uint32_t rd,                           \
                            std::uint32_t rs1,                           \
                            std::uint8_t shamt) {                        \
    assert(shamt < 64); /* shamt is 6 bits for RV64 (5 bits for RV32) */\
    return i_type(MATCH_##match, rd, rs1, shamt);                       \
  }

SHIFT_INSNS_LIST(DEFINE_SHIFT_INSN)
#undef DEFINE_SHIFT_INSN
#undef SHIFT_INSNS_LIST

//================================= U-type =================================//

static std::uint32_t
u_type(std::uint32_t op, std::uint32_t rd, std::uint32_t imm) {
  assert(op < (1u << 7));    // opcode is 7 bits
  assert(rd < (1u << 5));    // rd is 5 bits (register 0-31)
  assert(imm < (1u << 20));  // U-type immediate is 20 bits
  return (bits(imm, 19, 0) << 12) | (rd << 7) | op;
}

static std::uint32_t lui(std::uint32_t rd, std::uint32_t imm) {
  return u_type(MATCH_LUI, rd, imm);
}

//================================= B-type =================================//

static std::uint32_t b_type(std::uint32_t op,
                            std::uint32_t rs1,
                            std::uint32_t rs2,
                            std::uint32_t imm) {
  assert(op < (1u << 7));    // opcode is 7 bits
  assert(rs1 < (1u << 5));   // rs1 is 5 bits (register 0-31)
  assert(rs2 < (1u << 5));   // rs2 is 5 bits (register 0-31)
  assert(imm < (1u << 13));  // B-type immediate is 13 bits
  assert((imm & 1) == 0);    // B-type immediate must be 2-byte aligned
  return (bit(imm, 12) << 31) | (bits(imm, 10, 5) << 25) | (rs2 << 20) |
         (rs1 << 15) | (bits(imm, 4, 1) << 8) | (bit(imm, 11) << 7) | op;
}

//================================== CSRs ==================================//

static std::uint32_t csr_rtype(std::uint32_t op,
                               std::uint32_t rd,
                               std::uint32_t rs1,
                               std::uint32_t csr) {
  assert(op < (1u << 7));    // opcode is 7 bits
  assert(rd < (1u << 5));    // rd is 5 bits (register 0-31)
  assert(rs1 < (1u << 5));   // rs1 is 5 bits (register 0-31)
  assert(csr < (1u << 12));  // CSR address is 12 bits
  return (csr << 20) | (rs1 << 15) | (rd << 7) | op;
}

static std::uint32_t csr_itype(std::uint32_t op,
                               std::uint32_t rd,
                               std::uint32_t imm,
                               std::uint32_t csr) {
  assert(op < (1u << 7));    // opcode is 7 bits
  assert(rd < (1u << 5));    // rd is 5 bits (register 0-31)
  assert(imm < (1u << 5));   // CSR immediate (uimm) is 5 bits
  assert(csr < (1u << 12));  // CSR address is 12 bits
  return (csr << 20) | (bits(imm, 4, 0) << 15) | (rd << 7) | op;
}

static std::uint32_t
csrrw(std::uint32_t rd, std::uint32_t rs1, std::uint32_t csr) {
  return csr_rtype(MATCH_CSRRW, rd, rs1, csr);
}

// Define CSR register-based instructions using X-macro
#define CSR_REG_INSNS_LIST(INSN_MACRO) \
  INSN_MACRO(csrrs, CSRRS) \
  INSN_MACRO(csrrc, CSRRC)

#define DEFINE_CSR_REG_INSN(name, match)                                \
  static std::uint32_t name(std::uint32_t rd,                           \
                            std::uint32_t rs1,                           \
                            std::uint32_t csr) {                         \
    return csr_rtype(MATCH_##match, rd, rs1, csr);                      \
  }

CSR_REG_INSNS_LIST(DEFINE_CSR_REG_INSN)
#undef DEFINE_CSR_REG_INSN
#undef CSR_REG_INSNS_LIST

// Define CSR immediate-based instructions using X-macro
#define CSR_IMM_INSNS_LIST(INSN_MACRO) \
  INSN_MACRO(csrrwi, CSRRWI) \
  INSN_MACRO(csrrsi, CSRRSI) \
  INSN_MACRO(csrrci, CSRRCI)

#define DEFINE_CSR_IMM_INSN(name, match)                                \
  static std::uint32_t name(std::uint32_t rd,                           \
                            std::uint32_t imm,                           \
                            std::uint32_t csr) {                         \
    return csr_itype(MATCH_##match, rd, imm, csr);                      \
  }

CSR_IMM_INSNS_LIST(DEFINE_CSR_IMM_INSN)
#undef DEFINE_CSR_IMM_INSN
#undef CSR_IMM_INSNS_LIST

//================================= MISC =================================//

static std::uint32_t ebreak(void) {
  return MATCH_EBREAK;
}

static std::uint32_t ebreak_c(void) {
  return MATCH_C_EBREAK;
}

static std::uint32_t dret(void) {
  return MATCH_DRET;
}

static std::uint32_t fence_i(void) {
  return MATCH_FENCE_I;
}
