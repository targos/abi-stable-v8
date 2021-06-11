// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INCLUDED_FROM_MACRO_ASSEMBLER_H
#error This header must be included via macro-assembler.h
#endif

#ifndef V8_CODEGEN_S390_MACRO_ASSEMBLER_S390_H_
#define V8_CODEGEN_S390_MACRO_ASSEMBLER_S390_H_

#include "src/codegen/bailout-reason.h"
#include "src/codegen/s390/assembler-s390.h"
#include "src/common/globals.h"
#include "src/objects/contexts.h"

namespace v8 {
namespace internal {

enum class StackLimitKind { kInterruptStackLimit, kRealStackLimit };

// ----------------------------------------------------------------------------
// Static helper functions

// Generate a MemOperand for loading a field from an object.
inline MemOperand FieldMemOperand(Register object, int offset) {
  return MemOperand(object, offset - kHeapObjectTag);
}

// Generate a MemOperand for loading a field from an object.
inline MemOperand FieldMemOperand(Register object, Register index, int offset) {
  return MemOperand(object, index, offset - kHeapObjectTag);
}

enum LinkRegisterStatus { kLRHasNotBeenSaved, kLRHasBeenSaved };

Register GetRegisterThatIsNotOneOf(Register reg1, Register reg2 = no_reg,
                                   Register reg3 = no_reg,
                                   Register reg4 = no_reg,
                                   Register reg5 = no_reg,
                                   Register reg6 = no_reg);

class V8_EXPORT_PRIVATE TurboAssembler : public TurboAssemblerBase {
 public:
  using TurboAssemblerBase::TurboAssemblerBase;

  void AtomicCmpExchangeHelper(Register addr, Register output,
                               Register old_value, Register new_value,
                               int start, int end, int shift_amount, int offset,
                               Register temp0, Register temp1);
  void AtomicCmpExchangeU8(Register addr, Register output, Register old_value,
                           Register new_value, Register temp0, Register temp1);
  void AtomicCmpExchangeU16(Register addr, Register output, Register old_value,
                            Register new_value, Register temp0, Register temp1);
  void AtomicExchangeHelper(Register addr, Register value, Register output,
                            int start, int end, int shift_amount, int offset,
                            Register scratch);
  void AtomicExchangeU8(Register addr, Register value, Register output,
                        Register scratch);
  void AtomicExchangeU16(Register addr, Register value, Register output,
                         Register scratch);

  void DoubleMax(DoubleRegister result_reg, DoubleRegister left_reg,
                 DoubleRegister right_reg);
  void DoubleMin(DoubleRegister result_reg, DoubleRegister left_reg,
                 DoubleRegister right_reg);
  void FloatMax(DoubleRegister result_reg, DoubleRegister left_reg,
                DoubleRegister right_reg);
  void FloatMin(DoubleRegister result_reg, DoubleRegister left_reg,
                DoubleRegister right_reg);
  void CeilF32(DoubleRegister dst, DoubleRegister src);
  void CeilF64(DoubleRegister dst, DoubleRegister src);
  void FloorF32(DoubleRegister dst, DoubleRegister src);
  void FloorF64(DoubleRegister dst, DoubleRegister src);
  void TruncF32(DoubleRegister dst, DoubleRegister src);
  void TruncF64(DoubleRegister dst, DoubleRegister src);
  void NearestIntF32(DoubleRegister dst, DoubleRegister src);
  void NearestIntF64(DoubleRegister dst, DoubleRegister src);

  void LoadFromConstantsTable(Register destination, int constant_index) final;
  void LoadRootRegisterOffset(Register destination, intptr_t offset) final;
  void LoadRootRelative(Register destination, int32_t offset) final;

  // Jump, Call, and Ret pseudo instructions implementing inter-working.
  void Jump(Register target, Condition cond = al);
  void Jump(Address target, RelocInfo::Mode rmode, Condition cond = al);
  void Jump(Handle<Code> code, RelocInfo::Mode rmode, Condition cond = al);
  void Jump(const ExternalReference& reference);
  // Jump the register contains a smi.
  inline void JumpIfSmi(Register value, Label* smi_label) {
    TestIfSmi(value);
    beq(smi_label /*, cr0*/);  // branch if SMI
  }
  void JumpIfEqual(Register x, int32_t y, Label* dest);
  void JumpIfLessThan(Register x, int32_t y, Label* dest);

  void LoadMap(Register destination, Register object);

  void Call(Register target);
  void Call(Address target, RelocInfo::Mode rmode, Condition cond = al);
  void Call(Handle<Code> code, RelocInfo::Mode rmode = RelocInfo::CODE_TARGET,
            Condition cond = al);
  void Ret() { b(r14); }
  void Ret(Condition cond) { b(cond, r14); }

  void CallForDeoptimization(Builtin target, int deopt_id, Label* exit,
                             DeoptimizeKind kind, Label* ret,
                             Label* jump_deoptimization_entry_label);

  // Emit code to discard a non-negative number of pointer-sized elements
  // from the stack, clobbering only the sp register.
  void Drop(int count);
  void Drop(Register count, Register scratch = r0);

  void Ret(int drop) {
    Drop(drop);
    Ret();
  }

  void Call(Label* target);

  // Load the builtin given by the Smi in |builtin_index| into the same
  // register.
  void LoadEntryFromBuiltinIndex(Register builtin_index);
  void LoadCodeObjectEntry(Register destination, Register code_object);
  void CallCodeObject(Register code_object);
  void JumpCodeObject(Register code_object,
                      JumpMode jump_mode = JumpMode::kJump);

  void CallBuiltinByIndex(Register builtin_index);

  // Register move. May do nothing if the registers are identical.
  void Move(Register dst, Smi smi) { LoadSmiLiteral(dst, smi); }
  void Move(Register dst, Handle<HeapObject> source,
            RelocInfo::Mode rmode = RelocInfo::FULL_EMBEDDED_OBJECT);
  void Move(Register dst, ExternalReference reference);
  void Move(Register dst, Register src, Condition cond = al);
  void Move(DoubleRegister dst, DoubleRegister src);

  void MoveChar(const MemOperand& opnd1, const MemOperand& opnd2,
                const Operand& length);

  void CompareLogicalChar(const MemOperand& opnd1, const MemOperand& opnd2,
                          const Operand& length);

  void ExclusiveOrChar(const MemOperand& opnd1, const MemOperand& opnd2,
                       const Operand& length);

  void RotateInsertSelectBits(Register dst, Register src,
                              const Operand& startBit, const Operand& endBit,
                              const Operand& shiftAmt, bool zeroBits);

  void BranchRelativeOnIdxHighP(Register dst, Register inc, Label* L);

  void MaybeSaveRegisters(RegList registers);
  void MaybeRestoreRegisters(RegList registers);

  void CallEphemeronKeyBarrier(Register object, Register slot_address,
                               SaveFPRegsMode fp_mode);

  void CallRecordWriteStubSaveRegisters(
      Register object, Register slot_address,
      RememberedSetAction remembered_set_action, SaveFPRegsMode fp_mode,
      StubCallMode mode = StubCallMode::kCallBuiltinPointer);
  void CallRecordWriteStub(
      Register object, Register slot_address,
      RememberedSetAction remembered_set_action, SaveFPRegsMode fp_mode,
      StubCallMode mode = StubCallMode::kCallBuiltinPointer);

  void MultiPush(RegList regs, Register location = sp);
  void MultiPop(RegList regs, Register location = sp);

  void MultiPushDoubles(RegList dregs, Register location = sp);
  void MultiPopDoubles(RegList dregs, Register location = sp);

  void MultiPushV128(RegList dregs, Register location = sp);
  void MultiPopV128(RegList dregs, Register location = sp);

  // Calculate how much stack space (in bytes) are required to store caller
  // registers excluding those specified in the arguments.
  int RequiredStackSizeForCallerSaved(SaveFPRegsMode fp_mode,
                                      Register exclusion1 = no_reg,
                                      Register exclusion2 = no_reg,
                                      Register exclusion3 = no_reg) const;

  // Push caller saved registers on the stack, and return the number of bytes
  // stack pointer is adjusted.
  int PushCallerSaved(SaveFPRegsMode fp_mode, Register exclusion1 = no_reg,
                      Register exclusion2 = no_reg,
                      Register exclusion3 = no_reg);
  // Restore caller saved registers from the stack, and return the number of
  // bytes stack pointer is adjusted.
  int PopCallerSaved(SaveFPRegsMode fp_mode, Register exclusion1 = no_reg,
                     Register exclusion2 = no_reg,
                     Register exclusion3 = no_reg);

  // Load an object from the root table.
  void LoadRoot(Register destination, RootIndex index) override {
    LoadRoot(destination, index, al);
  }
  void LoadRoot(Register destination, RootIndex index, Condition cond);
  //--------------------------------------------------------------------------
  // S390 Macro Assemblers for Instructions
  //--------------------------------------------------------------------------

  // Arithmetic Operations

  // Add (Register - Immediate)
  void AddS32(Register dst, const Operand& imm);
  void AddS64(Register dst, const Operand& imm);
  void AddS32(Register dst, Register src, const Operand& imm);
  void AddS64(Register dst, Register src, const Operand& imm);
  void AddS32(Register dst, Register src, int32_t imm);
  void AddS64(Register dst, Register src, int32_t imm);

  // Add (Register - Register)
  void AddS32(Register dst, Register src);
  void AddS64(Register dst, Register src);
  void AddS32(Register dst, Register src1, Register src2);
  void AddS64(Register dst, Register src1, Register src2);

  // Add (Register - Mem)
  void AddS32(Register dst, const MemOperand& opnd);
  void AddS64(Register dst, const MemOperand& opnd);

  // Add (Mem - Immediate)
  void AddS32(const MemOperand& opnd, const Operand& imm);
  void AddS64(const MemOperand& opnd, const Operand& imm);

  // Add Logical (Register - Register)
  void AddU32(Register dst, Register src1, Register src2);

  // Add Logical (Register - Immediate)
  void AddU32(Register dst, const Operand& imm);
  void AddU64(Register dst, const Operand& imm);

  // Add Logical (Register - Mem)
  void AddU32(Register dst, const MemOperand& opnd);
  void AddU64(Register dst, const MemOperand& opnd);

  // Subtract (Register - Immediate)
  void SubS32(Register dst, const Operand& imm);
  void SubS64(Register dst, const Operand& imm);
  void SubS32(Register dst, Register src, const Operand& imm);
  void SubS64(Register dst, Register src, const Operand& imm);
  void SubS32(Register dst, Register src, int32_t imm);
  void SubS64(Register dst, Register src, int32_t imm);

  // Subtract (Register - Register)
  void SubS32(Register dst, Register src);
  void SubS64(Register dst, Register src);
  void SubS32(Register dst, Register src1, Register src2);
  void SubS64(Register dst, Register src1, Register src2);

  // Subtract (Register - Mem)
  void SubS32(Register dst, const MemOperand& opnd);
  void SubS64(Register dst, const MemOperand& opnd);
  void LoadAndSub32(Register dst, Register src, const MemOperand& opnd);
  void LoadAndSub64(Register dst, Register src, const MemOperand& opnd);

  // Subtract Logical (Register - Mem)
  void SubU32(Register dst, const MemOperand& opnd);
  void SubU64(Register dst, const MemOperand& opnd);
  // Subtract Logical 32-bit
  void SubU32(Register dst, Register src1, Register src2);

  // Multiply
  void MulS64(Register dst, const Operand& opnd);
  void MulS64(Register dst, Register src);
  void MulS64(Register dst, const MemOperand& opnd);
  void MulS64(Register dst, Register src1, Register src2) {
    if (CpuFeatures::IsSupported(MISC_INSTR_EXT2)) {
      msgrkc(dst, src1, src2);
    } else {
      if (dst == src2) {
        MulS64(dst, src1);
      } else if (dst == src1) {
        MulS64(dst, src2);
      } else {
        mov(dst, src1);
        MulS64(dst, src2);
      }
    }
  }

  void MulS32(Register dst, const MemOperand& src1);
  void MulS32(Register dst, Register src1);
  void MulS32(Register dst, const Operand& src1);
  void MulS32(Register dst, Register src1, Register src2) {
    if (CpuFeatures::IsSupported(MISC_INSTR_EXT2)) {
      msrkc(dst, src1, src2);
    } else {
      if (dst == src2) {
        MulS32(dst, src1);
      } else if (dst == src1) {
        MulS32(dst, src2);
      } else {
        mov(dst, src1);
        MulS32(dst, src2);
      }
    }
  }

  void MulHighS32(Register dst, Register src1, const MemOperand& src2);
  void MulHighS32(Register dst, Register src1, Register src2);
  void MulHighS32(Register dst, Register src1, const Operand& src2);
  void MulHighU32(Register dst, Register src1, const MemOperand& src2);
  void MulHighU32(Register dst, Register src1, Register src2);
  void MulHighU32(Register dst, Register src1, const Operand& src2);
  void Mul32WithOverflowIfCCUnequal(Register dst, Register src1,
                                    const MemOperand& src2);
  void Mul32WithOverflowIfCCUnequal(Register dst, Register src1, Register src2);
  void Mul32WithOverflowIfCCUnequal(Register dst, Register src1,
                                    const Operand& src2);
  // Divide
  void DivS32(Register dst, Register src1, const MemOperand& src2);
  void DivS32(Register dst, Register src1, Register src2);
  void DivU32(Register dst, Register src1, const MemOperand& src2);
  void DivU32(Register dst, Register src1, Register src2);
  void DivS64(Register dst, Register src1, const MemOperand& src2);
  void DivS64(Register dst, Register src1, Register src2);
  void DivU64(Register dst, Register src1, const MemOperand& src2);
  void DivU64(Register dst, Register src1, Register src2);

  // Mod
  void ModS32(Register dst, Register src1, const MemOperand& src2);
  void ModS32(Register dst, Register src1, Register src2);
  void ModU32(Register dst, Register src1, const MemOperand& src2);
  void ModU32(Register dst, Register src1, Register src2);
  void ModS64(Register dst, Register src1, const MemOperand& src2);
  void ModS64(Register dst, Register src1, Register src2);
  void ModU64(Register dst, Register src1, const MemOperand& src2);
  void ModU64(Register dst, Register src1, Register src2);

  // Square root
  void Sqrt(DoubleRegister result, DoubleRegister input);
  void Sqrt(DoubleRegister result, const MemOperand& input);

  // Compare
  void CmpS32(Register src1, Register src2);
  void CmpS64(Register src1, Register src2);
  void CmpS32(Register dst, const Operand& opnd);
  void CmpS64(Register dst, const Operand& opnd);
  void CmpS32(Register dst, const MemOperand& opnd);
  void CmpS64(Register dst, const MemOperand& opnd);
  void CmpAndSwap(Register old_val, Register new_val, const MemOperand& opnd);
  void CmpAndSwap64(Register old_val, Register new_val, const MemOperand& opnd);
  // TODO(john.yan): remove this
  template <class T>
  void CmpP(Register src1, T src2) {
    CmpS64(src1, src2);
  }

  // Compare Logical
  void CmpU32(Register src1, Register src2);
  void CmpU64(Register src1, Register src2);
  void CmpU32(Register src1, const Operand& opnd);
  void CmpU64(Register src1, const Operand& opnd);
  void CmpU32(Register dst, const MemOperand& opnd);
  void CmpU64(Register dst, const MemOperand& opnd);

  // Load
  void LoadU64(Register dst, const MemOperand& mem, Register scratch = no_reg);
  void LoadS32(Register dst, const MemOperand& opnd, Register scratch = no_reg);
  void LoadS32(Register dst, Register src);
  void LoadU32(Register dst, const MemOperand& opnd, Register scratch = no_reg);
  void LoadU32(Register dst, Register src);
  void LoadU16(Register dst, const MemOperand& opnd);
  void LoadU16(Register dst, Register src);
  void LoadS16(Register dst, Register src);
  void LoadS16(Register dst, const MemOperand& mem, Register scratch = no_reg);
  void LoadS8(Register dst, const MemOperand& opnd);
  void LoadS8(Register dst, Register src);
  void LoadU8(Register dst, const MemOperand& opnd);
  void LoadU8(Register dst, Register src);
  void LoadV128(Simd128Register dst, const MemOperand& mem, Register scratch);
  void LoadF64(DoubleRegister dst, const MemOperand& opnd);
  void LoadF32(DoubleRegister dst, const MemOperand& opnd);
  // LE Load
  void LoadU64LE(Register dst, const MemOperand& mem,
                 Register scratch = no_reg);
  void LoadS32LE(Register dst, const MemOperand& opnd,
                 Register scratch = no_reg);
  void LoadU32LE(Register dst, const MemOperand& opnd,
                 Register scratch = no_reg);
  void LoadU16LE(Register dst, const MemOperand& opnd);
  void LoadS16LE(Register dst, const MemOperand& opnd);
  void LoadV128LE(DoubleRegister dst, const MemOperand& mem, Register scratch0,
                  Register scratch1);
  void LoadF64LE(DoubleRegister dst, const MemOperand& opnd, Register scratch);
  void LoadF32LE(DoubleRegister dst, const MemOperand& opnd, Register scratch);

  // Load And Test
  void LoadAndTest32(Register dst, Register src);
  void LoadAndTestP(Register dst, Register src);

  void LoadAndTest32(Register dst, const MemOperand& opnd);
  void LoadAndTestP(Register dst, const MemOperand& opnd);

  // Store
  void StoreU64(const MemOperand& mem, const Operand& opnd,
                Register scratch = no_reg);
  void StoreU64(Register src, const MemOperand& mem, Register scratch = no_reg);
  void StoreU32(Register src, const MemOperand& mem, Register scratch = no_reg);

  void StoreU16(Register src, const MemOperand& mem, Register scratch = r0);
  void StoreU8(Register src, const MemOperand& mem, Register scratch = r0);
  void StoreF64(DoubleRegister dst, const MemOperand& opnd);
  void StoreF32(DoubleRegister dst, const MemOperand& opnd);
  void StoreV128(Simd128Register src, const MemOperand& mem, Register scratch);

  // Store LE
  void StoreU64LE(Register src, const MemOperand& mem,
                  Register scratch = no_reg);
  void StoreU32LE(Register src, const MemOperand& mem,
                  Register scratch = no_reg);

  void StoreU16LE(Register src, const MemOperand& mem, Register scratch = r0);
  void StoreF64LE(DoubleRegister src, const MemOperand& opnd, Register scratch);
  void StoreF32LE(DoubleRegister src, const MemOperand& opnd, Register scratch);
  void StoreV128LE(Simd128Register src, const MemOperand& mem,
                   Register scratch1, Register scratch2);

  void AddF32(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);
  void SubF32(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);
  void MulF32(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);
  void DivF32(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);

  void AddF64(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);
  void SubF64(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);
  void MulF64(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);
  void DivF64(DoubleRegister dst, DoubleRegister lhs, DoubleRegister rhs);

  void AddFloat32(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void AddFloat64(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void SubFloat32(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void SubFloat64(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void MulFloat32(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void MulFloat64(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void DivFloat32(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void DivFloat64(DoubleRegister dst, const MemOperand& opnd,
                  DoubleRegister scratch);
  void LoadF32AsF64(DoubleRegister dst, const MemOperand& opnd,
                    DoubleRegister scratch);

  // Load On Condition
  void LoadOnConditionP(Condition cond, Register dst, Register src);

  void LoadPositiveP(Register result, Register input);
  void LoadPositive32(Register result, Register input);

  void Branch(Condition c, const Operand& opnd);
  void BranchOnCount(Register r1, Label* l);

  // Shifts
  void ShiftLeftU32(Register dst, Register src, Register val,
                    const Operand& val2 = Operand::Zero());
  void ShiftLeftU32(Register dst, Register src, const Operand& val);
  void ShiftLeftU64(Register dst, Register src, Register val,
                    const Operand& val2 = Operand::Zero());
  void ShiftLeftU64(Register dst, Register src, const Operand& val);
  void ShiftRightU32(Register dst, Register src, Register val,
                     const Operand& val2 = Operand::Zero());
  void ShiftRightU32(Register dst, Register src, const Operand& val);
  void ShiftRightU64(Register dst, Register src, Register val,
                     const Operand& val2 = Operand::Zero());
  void ShiftRightU64(Register dst, Register src, const Operand& val);
  void ShiftRightS32(Register dst, Register src, Register shift,
                     const Operand& val2 = Operand::Zero());
  void ShiftRightS32(Register dst, Register src, const Operand& val);
  void ShiftRightS64(Register dst, Register src, Register shift,
                     const Operand& val2 = Operand::Zero());
  void ShiftRightS64(Register dst, Register src, const Operand& val);

  void ClearRightImm(Register dst, Register src, const Operand& val);

  // Bitwise operations
  void And(Register dst, Register src);
  void AndP(Register dst, Register src);
  void And(Register dst, Register src1, Register src2);
  void AndP(Register dst, Register src1, Register src2);
  void And(Register dst, const MemOperand& opnd);
  void AndP(Register dst, const MemOperand& opnd);
  void And(Register dst, const Operand& opnd);
  void AndP(Register dst, const Operand& opnd);
  void And(Register dst, Register src, const Operand& opnd);
  void AndP(Register dst, Register src, const Operand& opnd);
  void Or(Register dst, Register src);
  void OrP(Register dst, Register src);
  void Or(Register dst, Register src1, Register src2);
  void OrP(Register dst, Register src1, Register src2);
  void Or(Register dst, const MemOperand& opnd);
  void OrP(Register dst, const MemOperand& opnd);
  void Or(Register dst, const Operand& opnd);
  void OrP(Register dst, const Operand& opnd);
  void Or(Register dst, Register src, const Operand& opnd);
  void OrP(Register dst, Register src, const Operand& opnd);
  void Xor(Register dst, Register src);
  void XorP(Register dst, Register src);
  void Xor(Register dst, Register src1, Register src2);
  void XorP(Register dst, Register src1, Register src2);
  void Xor(Register dst, const MemOperand& opnd);
  void XorP(Register dst, const MemOperand& opnd);
  void Xor(Register dst, const Operand& opnd);
  void XorP(Register dst, const Operand& opnd);
  void Xor(Register dst, Register src, const Operand& opnd);
  void XorP(Register dst, Register src, const Operand& opnd);
  void Popcnt32(Register dst, Register src);
  void Not32(Register dst, Register src = no_reg);
  void Not64(Register dst, Register src = no_reg);
  void NotP(Register dst, Register src = no_reg);

#ifdef V8_TARGET_ARCH_S390X
  void Popcnt64(Register dst, Register src);
#endif

  void mov(Register dst, const Operand& src);
  void mov(Register dst, Register src);

  void CleanUInt32(Register x) {
#ifdef V8_TARGET_ARCH_S390X
    llgfr(x, x);
#endif
  }

  void push(DoubleRegister src) {
    lay(sp, MemOperand(sp, -kSystemPointerSize));
    StoreF64(src, MemOperand(sp));
  }

  void push(Register src) {
    lay(sp, MemOperand(sp, -kSystemPointerSize));
    StoreU64(src, MemOperand(sp));
  }

  void pop(DoubleRegister dst) {
    LoadF64(dst, MemOperand(sp));
    la(sp, MemOperand(sp, kSystemPointerSize));
  }

  void pop(Register dst) {
    LoadU64(dst, MemOperand(sp));
    la(sp, MemOperand(sp, kSystemPointerSize));
  }

  void pop() { la(sp, MemOperand(sp, kSystemPointerSize)); }

  void Push(Register src) { push(src); }

  // Push a handle.
  void Push(Handle<HeapObject> handle);
  void Push(Smi smi);

  // Push two registers.  Pushes leftmost register first (to highest address).
  void Push(Register src1, Register src2) {
    lay(sp, MemOperand(sp, -kSystemPointerSize * 2));
    StoreU64(src1, MemOperand(sp, kSystemPointerSize));
    StoreU64(src2, MemOperand(sp, 0));
  }

  // Push three registers.  Pushes leftmost register first (to highest address).
  void Push(Register src1, Register src2, Register src3) {
    lay(sp, MemOperand(sp, -kSystemPointerSize * 3));
    StoreU64(src1, MemOperand(sp, kSystemPointerSize * 2));
    StoreU64(src2, MemOperand(sp, kSystemPointerSize));
    StoreU64(src3, MemOperand(sp, 0));
  }

  // Push four registers.  Pushes leftmost register first (to highest address).
  void Push(Register src1, Register src2, Register src3, Register src4) {
    lay(sp, MemOperand(sp, -kSystemPointerSize * 4));
    StoreU64(src1, MemOperand(sp, kSystemPointerSize * 3));
    StoreU64(src2, MemOperand(sp, kSystemPointerSize * 2));
    StoreU64(src3, MemOperand(sp, kSystemPointerSize));
    StoreU64(src4, MemOperand(sp, 0));
  }

  // Push five registers.  Pushes leftmost register first (to highest address).
  void Push(Register src1, Register src2, Register src3, Register src4,
            Register src5) {
    DCHECK(src1 != src2);
    DCHECK(src1 != src3);
    DCHECK(src2 != src3);
    DCHECK(src1 != src4);
    DCHECK(src2 != src4);
    DCHECK(src3 != src4);
    DCHECK(src1 != src5);
    DCHECK(src2 != src5);
    DCHECK(src3 != src5);
    DCHECK(src4 != src5);

    lay(sp, MemOperand(sp, -kSystemPointerSize * 5));
    StoreU64(src1, MemOperand(sp, kSystemPointerSize * 4));
    StoreU64(src2, MemOperand(sp, kSystemPointerSize * 3));
    StoreU64(src3, MemOperand(sp, kSystemPointerSize * 2));
    StoreU64(src4, MemOperand(sp, kSystemPointerSize));
    StoreU64(src5, MemOperand(sp, 0));
  }

  enum PushArrayOrder { kNormal, kReverse };
  void PushArray(Register array, Register size, Register scratch,
                 Register scratch2, PushArrayOrder order = kNormal);

  void Pop(Register dst) { pop(dst); }

  // Pop two registers. Pops rightmost register first (from lower address).
  void Pop(Register src1, Register src2) {
    LoadU64(src2, MemOperand(sp, 0));
    LoadU64(src1, MemOperand(sp, kSystemPointerSize));
    la(sp, MemOperand(sp, 2 * kSystemPointerSize));
  }

  // Pop three registers.  Pops rightmost register first (from lower address).
  void Pop(Register src1, Register src2, Register src3) {
    LoadU64(src3, MemOperand(sp, 0));
    LoadU64(src2, MemOperand(sp, kSystemPointerSize));
    LoadU64(src1, MemOperand(sp, 2 * kSystemPointerSize));
    la(sp, MemOperand(sp, 3 * kSystemPointerSize));
  }

  // Pop four registers.  Pops rightmost register first (from lower address).
  void Pop(Register src1, Register src2, Register src3, Register src4) {
    LoadU64(src4, MemOperand(sp, 0));
    LoadU64(src3, MemOperand(sp, kSystemPointerSize));
    LoadU64(src2, MemOperand(sp, 2 * kSystemPointerSize));
    LoadU64(src1, MemOperand(sp, 3 * kSystemPointerSize));
    la(sp, MemOperand(sp, 4 * kSystemPointerSize));
  }

  // Pop five registers.  Pops rightmost register first (from lower address).
  void Pop(Register src1, Register src2, Register src3, Register src4,
           Register src5) {
    LoadU64(src5, MemOperand(sp, 0));
    LoadU64(src4, MemOperand(sp, kSystemPointerSize));
    LoadU64(src3, MemOperand(sp, 2 * kSystemPointerSize));
    LoadU64(src2, MemOperand(sp, 3 * kSystemPointerSize));
    LoadU64(src1, MemOperand(sp, 4 * kSystemPointerSize));
    la(sp, MemOperand(sp, 5 * kSystemPointerSize));
  }

  // Push a fixed frame, consisting of lr, fp, constant pool.
  void PushCommonFrame(Register marker_reg = no_reg);

  // Push a standard frame, consisting of lr, fp, constant pool,
  // context and JS function
  void PushStandardFrame(Register function_reg);

  void PopCommonFrame(Register marker_reg = no_reg);

  // Restore caller's frame pointer and return address prior to being
  // overwritten by tail call stack preparation.
  void RestoreFrameStateForTailCall();

  void InitializeRootRegister() {
    ExternalReference isolate_root = ExternalReference::isolate_root(isolate());
    mov(kRootRegister, Operand(isolate_root));
  }

  // If the value is a NaN, canonicalize the value else, do nothing.
  void CanonicalizeNaN(const DoubleRegister dst, const DoubleRegister src);
  void CanonicalizeNaN(const DoubleRegister value) {
    CanonicalizeNaN(value, value);
  }

  // Converts the integer (untagged smi) in |src| to a double, storing
  // the result to |dst|
  void ConvertIntToDouble(DoubleRegister dst, Register src);

  // Converts the unsigned integer (untagged smi) in |src| to
  // a double, storing the result to |dst|
  void ConvertUnsignedIntToDouble(DoubleRegister dst, Register src);

  // Converts the integer (untagged smi) in |src| to
  // a float, storing the result in |dst|
  void ConvertIntToFloat(DoubleRegister dst, Register src);

  // Converts the unsigned integer (untagged smi) in |src| to
  // a float, storing the result in |dst|
  void ConvertUnsignedIntToFloat(DoubleRegister dst, Register src);

  void ConvertInt64ToFloat(DoubleRegister double_dst, Register src);
  void ConvertInt64ToDouble(DoubleRegister double_dst, Register src);
  void ConvertUnsignedInt64ToFloat(DoubleRegister double_dst, Register src);
  void ConvertUnsignedInt64ToDouble(DoubleRegister double_dst, Register src);

  void MovIntToFloat(DoubleRegister dst, Register src);
  void MovFloatToInt(Register dst, DoubleRegister src);
  void MovDoubleToInt64(Register dst, DoubleRegister src);
  void MovInt64ToDouble(DoubleRegister dst, Register src);
  // Converts the double_input to an integer.  Note that, upon return,
  // the contents of double_dst will also hold the fixed point representation.
  void ConvertFloat32ToInt64(const Register dst,
                             const DoubleRegister double_input,
                             FPRoundingMode rounding_mode = kRoundToZero);

  // Converts the double_input to an integer.  Note that, upon return,
  // the contents of double_dst will also hold the fixed point representation.
  void ConvertDoubleToInt64(const Register dst,
                            const DoubleRegister double_input,
                            FPRoundingMode rounding_mode = kRoundToZero);
  void ConvertDoubleToInt32(const Register dst,
                            const DoubleRegister double_input,
                            FPRoundingMode rounding_mode = kRoundToZero);

  void ConvertFloat32ToInt32(const Register result,
                             const DoubleRegister double_input,
                             FPRoundingMode rounding_mode);
  void ConvertFloat32ToUnsignedInt32(
      const Register result, const DoubleRegister double_input,
      FPRoundingMode rounding_mode = kRoundToZero);
  // Converts the double_input to an unsigned integer.  Note that, upon return,
  // the contents of double_dst will also hold the fixed point representation.
  void ConvertDoubleToUnsignedInt64(
      const Register dst, const DoubleRegister double_input,
      FPRoundingMode rounding_mode = kRoundToZero);
  void ConvertDoubleToUnsignedInt32(
      const Register dst, const DoubleRegister double_input,
      FPRoundingMode rounding_mode = kRoundToZero);
  void ConvertFloat32ToUnsignedInt64(
      const Register result, const DoubleRegister double_input,
      FPRoundingMode rounding_mode = kRoundToZero);

  // Generates function and stub prologue code.
  void StubPrologue(StackFrame::Type type, Register base = no_reg,
                    int prologue_offset = 0);
  void Prologue(Register base, int prologue_offset = 0);

  // Get the actual activation frame alignment for target environment.
  static int ActivationFrameAlignment();
  // ----------------------------------------------------------------
  // new S390 macro-assembler interfaces that are slightly higher level
  // than assembler-s390 and may generate variable length sequences

  // load an SMI value <value> to GPR <dst>
  void LoadSmiLiteral(Register dst, Smi smi);

  // load a literal double value <value> to FPR <result>
  template <class T>
  void LoadF64(DoubleRegister result, T value, Register scratch) {
    static_assert(sizeof(T) == kDoubleSize, "Expect input size to be 8");
    uint64_t int_val = bit_cast<uint64_t, T>(value);
    // Load the 64-bit value into a GPR, then transfer it to FPR via LDGR
    uint32_t hi_32 = int_val >> 32;
    uint32_t lo_32 = static_cast<uint32_t>(int_val);

    if (int_val == 0) {
      lzdr(result);
    } else if (lo_32 == 0) {
      llihf(scratch, Operand(hi_32));
      ldgr(result, scratch);
    } else {
      iihf(scratch, Operand(hi_32));
      iilf(scratch, Operand(lo_32));
      ldgr(result, scratch);
    }
  }

  template <class T>
  void LoadF32(DoubleRegister result, T value, Register scratch) {
    static_assert(sizeof(T) == kFloatSize, "Expect input size to be 4");
    uint32_t int_val = bit_cast<uint32_t, T>(value);
    LoadF64(result, static_cast<uint64_t>(int_val) << 32, scratch);
  }

  void CmpSmiLiteral(Register src1, Smi smi, Register scratch);

  // Set new rounding mode RN to FPSCR
  void SetRoundingMode(FPRoundingMode RN);

  // reset rounding mode to default (kRoundToNearest)
  void ResetRoundingMode();

  // These exist to provide portability between 32 and 64bit
  void LoadMultipleP(Register dst1, Register dst2, const MemOperand& mem);
  void StoreMultipleP(Register dst1, Register dst2, const MemOperand& mem);
  void LoadMultipleW(Register dst1, Register dst2, const MemOperand& mem);
  void StoreMultipleW(Register dst1, Register dst2, const MemOperand& mem);

  void SwapP(Register src, Register dst, Register scratch);
  void SwapP(Register src, MemOperand dst, Register scratch);
  void SwapP(MemOperand src, MemOperand dst, Register scratch_0,
             Register scratch_1);
  void SwapFloat32(DoubleRegister src, DoubleRegister dst,
                   DoubleRegister scratch);
  void SwapFloat32(DoubleRegister src, MemOperand dst, DoubleRegister scratch);
  void SwapFloat32(MemOperand src, MemOperand dst, DoubleRegister scratch);
  void SwapDouble(DoubleRegister src, DoubleRegister dst,
                  DoubleRegister scratch);
  void SwapDouble(DoubleRegister src, MemOperand dst, DoubleRegister scratch);
  void SwapDouble(MemOperand src, MemOperand dst, DoubleRegister scratch);
  void SwapSimd128(Simd128Register src, Simd128Register dst,
                   Simd128Register scratch);
  void SwapSimd128(Simd128Register src, MemOperand dst,
                   Simd128Register scratch);
  void SwapSimd128(MemOperand src, MemOperand dst, Simd128Register scratch);

  // Cleanse pointer address on 31bit by zero out top  bit.
  // This is a NOP on 64-bit.
  void CleanseP(Register src) {
#if (V8_HOST_ARCH_S390 && !(V8_TARGET_ARCH_S390X))
    nilh(src, Operand(0x7FFF));
#endif
  }

  void PrepareForTailCall(Register callee_args_count,
                          Register caller_args_count, Register scratch0,
                          Register scratch1);

  // ---------------------------------------------------------------------------
  // Runtime calls

  // Before calling a C-function from generated code, align arguments on stack.
  // After aligning the frame, non-register arguments must be stored in
  // sp[0], sp[4], etc., not pushed. The argument count assumes all arguments
  // are word sized. If double arguments are used, this function assumes that
  // all double arguments are stored before core registers; otherwise the
  // correct alignment of the double values is not guaranteed.
  // Some compilers/platforms require the stack to be aligned when calling
  // C++ code.
  // Needs a scratch register to do some arithmetic. This register will be
  // trashed.
  void PrepareCallCFunction(int num_reg_arguments, int num_double_registers,
                            Register scratch);
  void PrepareCallCFunction(int num_reg_arguments, Register scratch);

  // There are two ways of passing double arguments on ARM, depending on
  // whether soft or hard floating point ABI is used. These functions
  // abstract parameter passing for the three different ways we call
  // C functions from generated code.
  void MovToFloatParameter(DoubleRegister src);
  void MovToFloatParameters(DoubleRegister src1, DoubleRegister src2);
  void MovToFloatResult(DoubleRegister src);

  // Calls a C function and cleans up the space for arguments allocated
  // by PrepareCallCFunction. The called function is not allowed to trigger a
  // garbage collection, since that might move the code and invalidate the
  // return address (unless this is somehow accounted for by the called
  // function).
  void CallCFunction(ExternalReference function, int num_arguments);
  void CallCFunction(Register function, int num_arguments);
  void CallCFunction(ExternalReference function, int num_reg_arguments,
                     int num_double_arguments);
  void CallCFunction(Register function, int num_reg_arguments,
                     int num_double_arguments);

  void MovFromFloatParameter(DoubleRegister dst);
  void MovFromFloatResult(DoubleRegister dst);

  void Trap();
  void DebugBreak();

  // Emit code for a truncating division by a constant. The dividend register is
  // unchanged and ip gets clobbered. Dividend and result must be different.
  void TruncateDoubleToI(Isolate* isolate, Zone* zone, Register result,
                         DoubleRegister double_input, StubCallMode stub_mode);
  void TryInlineTruncateDoubleToI(Register result, DoubleRegister double_input,
                                  Label* done);

  // ---------------------------------------------------------------------------
  // Debugging

  // Calls Abort(msg) if the condition cond is not satisfied.
  // Use --debug_code to enable.
  void Assert(Condition cond, AbortReason reason, CRegister cr = cr7);

  // Like Assert(), but without condition.
  // Use --debug-code to enable.
  void AssertUnreachable(AbortReason reason);

  // Like Assert(), but always enabled.
  void Check(Condition cond, AbortReason reason, CRegister cr = cr7);

  // Print a message to stdout and abort execution.
  void Abort(AbortReason reason);

  // ---------------------------------------------------------------------------
  // Bit testing/extraction
  //
  // Bit numbering is such that the least significant bit is bit 0
  // (for consistency between 32/64-bit).

  // Extract consecutive bits (defined by rangeStart - rangeEnd) from src
  // and place them into the least significant bits of dst.
  inline void ExtractBitRange(Register dst, Register src, int rangeStart,
                              int rangeEnd) {
    DCHECK(rangeStart >= rangeEnd && rangeStart < kBitsPerSystemPointer);

    // Try to use RISBG if possible.
    if (CpuFeatures::IsSupported(GENERAL_INSTR_EXT)) {
      int shiftAmount = (64 - rangeEnd) % 64;  // Convert to shift left.
      int endBit = 63;  // End is always LSB after shifting.
      int startBit = 63 - rangeStart + rangeEnd;
      RotateInsertSelectBits(dst, src, Operand(startBit), Operand(endBit),
                             Operand(shiftAmount), true);
    } else {
      if (rangeEnd > 0)  // Don't need to shift if rangeEnd is zero.
        ShiftRightU64(dst, src, Operand(rangeEnd));
      else if (dst != src)  // If we didn't shift, we might need to copy
        mov(dst, src);
      int width = rangeStart - rangeEnd + 1;
#if V8_TARGET_ARCH_S390X
      uint64_t mask = (static_cast<uint64_t>(1) << width) - 1;
      nihf(dst, Operand(mask >> 32));
      nilf(dst, Operand(mask & 0xFFFFFFFF));
      ltgr(dst, dst);
#else
      uint32_t mask = (1 << width) - 1;
      AndP(dst, Operand(mask));
#endif
    }
  }

  inline void ExtractBit(Register dst, Register src, uint32_t bitNumber) {
    ExtractBitRange(dst, src, bitNumber, bitNumber);
  }

  // Extract consecutive bits (defined by mask) from src and place them
  // into the least significant bits of dst.
  inline void ExtractBitMask(Register dst, Register src, uintptr_t mask,
                             RCBit rc = LeaveRC) {
    int start = kBitsPerSystemPointer - 1;
    int end;
    uintptr_t bit = (1L << start);

    while (bit && (mask & bit) == 0) {
      start--;
      bit >>= 1;
    }
    end = start;
    bit >>= 1;

    while (bit && (mask & bit)) {
      end--;
      bit >>= 1;
    }

    // 1-bits in mask must be contiguous
    DCHECK(bit == 0 || (mask & ((bit << 1) - 1)) == 0);

    ExtractBitRange(dst, src, start, end);
  }

  // Test single bit in value.
  inline void TestBit(Register value, int bitNumber, Register scratch = r0) {
    ExtractBitRange(scratch, value, bitNumber, bitNumber);
  }

  // Test consecutive bit range in value.  Range is defined by
  // rangeStart - rangeEnd.
  inline void TestBitRange(Register value, int rangeStart, int rangeEnd,
                           Register scratch = r0) {
    ExtractBitRange(scratch, value, rangeStart, rangeEnd);
  }

  // Test consecutive bit range in value.  Range is defined by mask.
  inline void TestBitMask(Register value, uintptr_t mask,
                          Register scratch = r0) {
    ExtractBitMask(scratch, value, mask, SetRC);
  }
  inline void TestIfSmi(Register value) { tmll(value, Operand(1)); }

  inline void TestIfSmi(MemOperand value) {
    if (is_uint12(value.offset())) {
      tm(value, Operand(1));
    } else if (is_int20(value.offset())) {
      tmy(value, Operand(1));
    } else {
      LoadS8(r0, value);
      tmll(r0, Operand(1));
    }
  }

  inline void TestIfInt32(Register value) {
    // High bits must be identical to fit into an 32-bit integer
    cgfr(value, value);
  }
  void SmiUntag(Register reg) { SmiUntag(reg, reg); }

  void SmiUntag(Register dst, const MemOperand& src);
  void SmiUntag(Register dst, Register src) {
    if (SmiValuesAre31Bits()) {
      ShiftRightS32(dst, src, Operand(kSmiShift));
    } else {
      ShiftRightS64(dst, src, Operand(kSmiShift));
    }
    lgfr(dst, dst);
  }

  // Activation support.
  void EnterFrame(StackFrame::Type type,
                  bool load_constant_pool_pointer_reg = false);
  // Returns the pc offset at which the frame ends.
  int LeaveFrame(StackFrame::Type type, int stack_adjustment = 0);

  void AllocateStackSpace(int bytes) {
    DCHECK_GE(bytes, 0);
    if (bytes == 0) return;
    lay(sp, MemOperand(sp, -bytes));
  }

  void CheckPageFlag(Register object, Register scratch, int mask, Condition cc,
                     Label* condition_met);

  void ResetSpeculationPoisonRegister();
  void ComputeCodeStartAddress(Register dst);
  void LoadPC(Register dst);

  // Control-flow integrity:

  // Define a function entrypoint. This doesn't emit any code for this
  // architecture, as control-flow integrity is not supported for it.
  void CodeEntry() {}
  // Define an exception handler.
  void ExceptionHandler() {}
  // Define an exception handler and bind a label.
  void BindExceptionHandler(Label* label) { bind(label); }

  // Generates an instruction sequence s.t. the return address points to the
  // instruction following the call.
  // The return address on the stack is used by frame iteration.
  void StoreReturnAddressAndCall(Register target);

  // ---------------------------------------------------------------------------
  // Pointer compression Support

  // Loads a field containing a HeapObject and decompresses it if pointer
  // compression is enabled.
  void LoadTaggedPointerField(const Register& destination,
                              const MemOperand& field_operand,
                              const Register& scratch = no_reg);

  // Loads a field containing any tagged value and decompresses it if necessary.
  void LoadAnyTaggedField(const Register& destination,
                          const MemOperand& field_operand,
                          const Register& scratch = no_reg);

  // Loads a field containing smi value and untags it.
  void SmiUntagField(Register dst, const MemOperand& src);

  // Compresses and stores tagged value to given on-heap location.
  void StoreTaggedField(const Register& value,
                        const MemOperand& dst_field_operand,
                        const Register& scratch = no_reg);

  void DecompressTaggedSigned(Register destination, MemOperand field_operand);
  void DecompressTaggedSigned(Register destination, Register src);
  void DecompressTaggedPointer(Register destination, MemOperand field_operand);
  void DecompressTaggedPointer(Register destination, Register source);
  void DecompressAnyTagged(Register destination, MemOperand field_operand);
  void DecompressAnyTagged(Register destination, Register source);

  // CountLeadingZeros will corrupt the scratch register pair (eg. r0:r1)
  void CountLeadingZerosU32(Register dst, Register src,
                            Register scratch_pair = r0);
  void CountLeadingZerosU64(Register dst, Register src,
                            Register scratch_pair = r0);
  void CountTrailingZerosU32(Register dst, Register src,
                             Register scratch_pair = r0);
  void CountTrailingZerosU64(Register dst, Register src,
                             Register scratch_pair = r0);

 private:
  static const int kSmiShift = kSmiTagSize + kSmiShiftSize;

  void CallCFunctionHelper(Register function, int num_reg_arguments,
                           int num_double_arguments);

  void Jump(intptr_t target, RelocInfo::Mode rmode, Condition cond = al);
  int CalculateStackPassedWords(int num_reg_arguments,
                                int num_double_arguments);
};

// MacroAssembler implements a collection of frequently used macros.
class V8_EXPORT_PRIVATE MacroAssembler : public TurboAssembler {
 public:
  using TurboAssembler::TurboAssembler;

  // It assumes that the arguments are located below the stack pointer.
  // argc is the number of arguments not including the receiver.
  // TODO(victorgomes): Remove this function once we stick with the reversed
  // arguments order.
  void LoadReceiver(Register dest, Register argc) {
    LoadU64(dest, MemOperand(sp, 0));
  }

  void StoreReceiver(Register rec, Register argc, Register scratch) {
    StoreU64(rec, MemOperand(sp, 0));
  }

  void CallRuntime(const Runtime::Function* f, int num_arguments,
                   SaveFPRegsMode save_doubles = SaveFPRegsMode::kIgnore);
  void CallRuntimeSaveDoubles(Runtime::FunctionId fid) {
    const Runtime::Function* function = Runtime::FunctionForId(fid);
    CallRuntime(function, function->nargs, SaveFPRegsMode::kSave);
  }

  // Convenience function: Same as above, but takes the fid instead.
  void CallRuntime(Runtime::FunctionId fid,
                   SaveFPRegsMode save_doubles = SaveFPRegsMode::kIgnore) {
    const Runtime::Function* function = Runtime::FunctionForId(fid);
    CallRuntime(function, function->nargs, save_doubles);
  }

  // Convenience function: Same as above, but takes the fid instead.
  void CallRuntime(Runtime::FunctionId fid, int num_arguments,
                   SaveFPRegsMode save_doubles = SaveFPRegsMode::kIgnore) {
    CallRuntime(Runtime::FunctionForId(fid), num_arguments, save_doubles);
  }

  // Convenience function: tail call a runtime routine (jump).
  void TailCallRuntime(Runtime::FunctionId fid);

  // ---------------------------------------------------------------------------
  // Support functions.

  // Compare object type for heap object.  heap_object contains a non-Smi
  // whose object type should be compared with the given type.  This both
  // sets the flags and leaves the object type in the type_reg register.
  // It leaves the map in the map register (unless the type_reg and map register
  // are the same register).  It leaves the heap object in the heap_object
  // register unless the heap_object register is the same register as one of the
  // other registers.
  // Type_reg can be no_reg. In that case ip is used.
  void CompareObjectType(Register heap_object, Register map, Register type_reg,
                         InstanceType type);

  // Compare instance type in a map.  map contains a valid map object whose
  // object type should be compared with the given type.  This both
  // sets the flags and leaves the object type in the type_reg register.
  void CompareInstanceType(Register map, Register type_reg, InstanceType type);

  // Compare instance type ranges for a map (lower_limit and higher_limit
  // inclusive).
  //
  // Always use unsigned comparisons: ls for a positive result.
  void CompareInstanceTypeRange(Register map, Register type_reg,
                                InstanceType lower_limit,
                                InstanceType higher_limit);

  // Compare the object in a register to a value from the root list.
  // Uses the ip register as scratch.
  void CompareRoot(Register obj, RootIndex index);
  void PushRoot(RootIndex index) {
    LoadRoot(r0, index);
    Push(r0);
  }

  template <class T>
  void CompareTagged(Register src1, T src2) {
    if (COMPRESS_POINTERS_BOOL) {
      CmpS32(src1, src2);
    } else {
      CmpS64(src1, src2);
    }
  }

  // Jump to a runtime routine.
  void JumpToExternalReference(const ExternalReference& builtin,
                               bool builtin_exit_frame = false);

  // Generates a trampoline to jump to the off-heap instruction stream.
  void JumpToInstructionStream(Address entry);

  // Compare the object in a register to a value and jump if they are equal.
  void JumpIfRoot(Register with, RootIndex index, Label* if_equal) {
    CompareRoot(with, index);
    beq(if_equal);
  }

  // Compare the object in a register to a value and jump if they are not equal.
  void JumpIfNotRoot(Register with, RootIndex index, Label* if_not_equal) {
    CompareRoot(with, index);
    bne(if_not_equal);
  }

  // Checks if value is in range [lower_limit, higher_limit] using a single
  // comparison.
  void JumpIfIsInRange(Register value, unsigned lower_limit,
                       unsigned higher_limit, Label* on_in_range);

  // ---------------------------------------------------------------------------
  // In-place weak references.
  void LoadWeakValue(Register out, Register in, Label* target_if_cleared);

  // ---------------------------------------------------------------------------
  // StatsCounter support

  void IncrementCounter(StatsCounter* counter, int value, Register scratch1,
                        Register scratch2) {
    if (!FLAG_native_code_counters) return;
    EmitIncrementCounter(counter, value, scratch1, scratch2);
  }
  void EmitIncrementCounter(StatsCounter* counter, int value, Register scratch1,
                            Register scratch2);
  void DecrementCounter(StatsCounter* counter, int value, Register scratch1,
                        Register scratch2) {
    if (!FLAG_native_code_counters) return;
    EmitDecrementCounter(counter, value, scratch1, scratch2);
  }
  void EmitDecrementCounter(StatsCounter* counter, int value, Register scratch1,
                            Register scratch2);

  // ---------------------------------------------------------------------------
  // Stack limit utilities

  MemOperand StackLimitAsMemOperand(StackLimitKind kind);
  void StackOverflowCheck(Register num_args, Register scratch,
                          Label* stack_overflow);

  // ---------------------------------------------------------------------------
  // JavaScript invokes

  // Set up call kind marking in ecx. The method takes ecx as an
  // explicit first parameter to make the code more readable at the
  // call sites.
  // void SetCallKind(Register dst, CallKind kind);

  // Removes current frame and its arguments from the stack preserving
  // the arguments and a return address pushed to the stack for the next call.
  // Both |callee_args_count| and |caller_args_count| do not include
  // receiver. |callee_args_count| is not modified. |caller_args_count|
  // is trashed.

  // Invoke the JavaScript function code by either calling or jumping.
  void InvokeFunctionCode(Register function, Register new_target,
                          Register expected_parameter_count,
                          Register actual_parameter_count, InvokeType type);

  // On function call, call into the debugger if necessary.
  void CheckDebugHook(Register fun, Register new_target,
                      Register expected_parameter_count,
                      Register actual_parameter_count);

  // Invoke the JavaScript function in the given register. Changes the
  // current context to the context in the function before invoking.
  void InvokeFunctionWithNewTarget(Register function, Register new_target,
                                   Register actual_parameter_count,
                                   InvokeType type);
  void InvokeFunction(Register function, Register expected_parameter_count,
                      Register actual_parameter_count, InvokeType type);

  // Exception handling

  // Push a new stack handler and link into stack handler chain.
  void PushStackHandler();

  // Unlink the stack handler on top of the stack from the stack handler chain.
  // Must preserve the result register.
  void PopStackHandler();

  // Enter exit frame.
  // stack_space - extra stack space, used for parameters before call to C.
  // At least one slot (for the return address) should be provided.
  void EnterExitFrame(bool save_doubles, int stack_space = 1,
                      StackFrame::Type frame_type = StackFrame::EXIT);

  // Leave the current exit frame. Expects the return value in r0.
  // Expect the number of values, pushed prior to the exit frame, to
  // remove in a register (or no_reg, if there is nothing to remove).
  void LeaveExitFrame(bool save_doubles, Register argument_count,
                      bool argument_count_is_length = false);

  // Load the global proxy from the current context.
  void LoadGlobalProxy(Register dst) {
    LoadNativeContextSlot(dst, Context::GLOBAL_PROXY_INDEX);
  }

  void LoadNativeContextSlot(Register dst, int index);

  // ---------------------------------------------------------------------------
  // Smi utilities

  // Shift left by kSmiShift
  void SmiTag(Register reg) { SmiTag(reg, reg); }
  void SmiTag(Register dst, Register src) {
    ShiftLeftU64(dst, src, Operand(kSmiShift));
  }

  void SmiToPtrArrayOffset(Register dst, Register src) {
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
    STATIC_ASSERT(kSmiTag == 0 && kSmiShift < kSystemPointerSizeLog2);
    ShiftLeftU64(dst, src, Operand(kSystemPointerSizeLog2 - kSmiShift));
#else
    STATIC_ASSERT(kSmiTag == 0 && kSmiShift > kSystemPointerSizeLog2);
    ShiftRightS64(dst, src, Operand(kSmiShift - kSystemPointerSizeLog2));
#endif
  }

  // Jump if either of the registers contain a non-smi.
  inline void JumpIfNotSmi(Register value, Label* not_smi_label) {
    TestIfSmi(value);
    bne(not_smi_label /*, cr0*/);
  }

  // Abort execution if argument is a smi, enabled via --debug-code.
  void AssertNotSmi(Register object);
  void AssertSmi(Register object);

#if !defined(V8_COMPRESS_POINTERS) && !defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  // Ensure it is permissible to read/write int value directly from
  // upper half of the smi.
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiTagSize + kSmiShiftSize == 32);
#endif
#if V8_TARGET_LITTLE_ENDIAN
#define SmiWordOffset(offset) (offset + kSystemPointerSize / 2)
#else
#define SmiWordOffset(offset) offset
#endif

  // Abort execution if argument is not a Constructor, enabled via --debug-code.
  void AssertConstructor(Register object, Register scratch);

  // Abort execution if argument is not a JSFunction, enabled via --debug-code.
  void AssertFunction(Register object);

  // Abort execution if argument is not a JSBoundFunction,
  // enabled via --debug-code.
  void AssertBoundFunction(Register object);

  // Abort execution if argument is not a JSGeneratorObject (or subclass),
  // enabled via --debug-code.
  void AssertGeneratorObject(Register object);

  // Abort execution if argument is not undefined or an AllocationSite, enabled
  // via --debug-code.
  void AssertUndefinedOrAllocationSite(Register object, Register scratch);

  template <typename Field>
  void DecodeField(Register dst, Register src) {
    ExtractBitRange(dst, src, Field::kShift + Field::kSize - 1, Field::kShift);
  }

  template <typename Field>
  void DecodeField(Register reg) {
    DecodeField<Field>(reg, reg);
  }

  // ---------------------------------------------------------------------------
  // GC Support

  void IncrementalMarkingRecordWriteHelper(Register object, Register value,
                                           Register address);

  void CallJSEntry(Register target);
  static int CallSizeNotPredictableCodeSize(Address target,
                                            RelocInfo::Mode rmode,
                                            Condition cond = al);
  // Notify the garbage collector that we wrote a pointer into an object.
  // |object| is the object being stored into, |value| is the object being
  // stored.  value and scratch registers are clobbered by the operation.
  // The offset is the offset from the start of the object, not the offset from
  // the tagged HeapObject pointer.  For use with FieldMemOperand(reg, off).
  void RecordWriteField(
      Register object, int offset, Register value, Register scratch,
      LinkRegisterStatus lr_status, SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = RememberedSetAction::kEmit,
      SmiCheck smi_check = SmiCheck::kInline);

  // For a given |object| notify the garbage collector that the slot |address|
  // has been written.  |value| is the object being stored. The value and
  // address registers are clobbered by the operation.
  void RecordWrite(
      Register object, Register address, Register value,
      LinkRegisterStatus lr_status, SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = RememberedSetAction::kEmit,
      SmiCheck smi_check = SmiCheck::kInline);

 private:
  static const int kSmiShift = kSmiTagSize + kSmiShiftSize;
  // Helper functions for generating invokes.
  void InvokePrologue(Register expected_parameter_count,
                      Register actual_parameter_count, Label* done,
                      InvokeType type);

  DISALLOW_IMPLICIT_CONSTRUCTORS(MacroAssembler);
};

#define ACCESS_MASM(masm) masm->

}  // namespace internal
}  // namespace v8

#endif  // V8_CODEGEN_S390_MACRO_ASSEMBLER_S390_H_
