#include "x86_calling_convention.hpp"

MEDUSA_NAMESPACE_USE

// cdecl //////////////////////////////////////////////////////////////////////

bool CdeclCallingConvention::GetIntParameter(CpuContext const* pCpuCtxt, MemoryContext const* pMemCtxt, u16 ParamNr, IntType& rParamValue) const
{
  switch (m_Mode)
  {
  case X86_Bit_16:
  {
    u16 StkSeg;
    u16 StkOff;
    if (!pCpuCtxt->ReadRegister(X86_Reg_Ss, StkSeg));
    if (!pCpuCtxt->ReadRegister(X86_Reg_Sp, StkOff))
      return false;
    u64 StkLinAddr;
    if (!pCpuCtxt->Translate(Address(StkSeg, StkOff), StkLinAddr))
      return false;
    u16 ParamVal;
    if (!pMemCtxt->ReadMemory(StkLinAddr + (ParamNr + 1) * 2, ParamVal))
      return false;
    rParamValue = IntType(ParamVal);
    return true;
  }

  case X86_Bit_32:
  {
    u16 StkSeg;
    u32 StkOff;
    if (!pCpuCtxt->ReadRegister(X86_Reg_Ss, StkSeg));
    if (!pCpuCtxt->ReadRegister(X86_Reg_Esp, StkOff))
      return false;
    u64 StkLinAddr;
    if (!pCpuCtxt->Translate(Address(StkSeg, StkOff), StkLinAddr))
      return false;
    u32 ParamVal;
    if (!pMemCtxt->ReadMemory(StkLinAddr + (ParamNr + 1) * 4, ParamVal))
      return false;
    rParamValue = IntType(ParamVal);
    return true;
  }

  default:
    return false;
  }
}

bool CdeclCallingConvention::ReturnFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo) const
{
  switch (m_Mode)
  {
  case X86_Bit_16:
  {
    u16 StkSeg;
    u16 StkOff;
    if (!pCpuCtxt->ReadRegister(X86_Reg_Ss, StkSeg));
    if (!pCpuCtxt->ReadRegister(X86_Reg_Sp, StkOff))
      return false;
    u64 StkLinAddr;
    if (!pCpuCtxt->Translate(Address(StkSeg, StkOff), StkLinAddr))
      return false;
    u16 RetVal;
    if (!pMemCtxt->ReadMemory(StkLinAddr, RetVal))
      return false;
    if (!pCpuCtxt->WriteRegister(X86_Reg_Ip, RetVal))
      return false;
    StkOff += 2;
    if (!pCpuCtxt->WriteRegister(X86_Reg_Sp, StkOff))
      return false;
    return true;
  }

  case X86_Bit_32:
  {
    u16 StkSeg;
    u32 StkOff;
    if (!pCpuCtxt->ReadRegister(X86_Reg_Ss, StkSeg));
    if (!pCpuCtxt->ReadRegister(X86_Reg_Esp, StkOff))
      return false;
    u64 StkLinAddr;
    if (!pCpuCtxt->Translate(Address(StkSeg, StkOff), StkLinAddr))
      return false;
    u32 RetVal;
    if (!pMemCtxt->ReadMemory(StkLinAddr, RetVal))
      return false;
    if (!pCpuCtxt->WriteRegister(X86_Reg_Eip, RetVal))
      return false;
    StkOff += 4;
    if (!pCpuCtxt->WriteRegister(X86_Reg_Esp, StkOff))
      return false;
    return true;
  }

  default:
    return false;
  }
}

bool CdeclCallingConvention::ReturnValueFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo, IntType const& rRetVal) const
{
  if (!ReturnFromFunction(pCpuCtxt, pMemCtxt, ParamNo))
    return false;
  switch (m_Mode)
  {
  case X86_Bit_16: return pCpuCtxt->WriteRegister(X86_Reg_Ax, rRetVal);
  case X86_Bit_32: return pCpuCtxt->WriteRegister(X86_Reg_Eax, rRetVal);
  default:         return false;
  }
}

Expression::SPType CdeclCallingConvention::EmitGetIntParameter(u16 ParamNr) const
{
  switch (m_Mode)
  {
  case X86_Bit_16: return Expr::MakeMem(16, Expr::MakeId(X86_Reg_Ss, &m_rCpuInfo),
    Expr::MakeBinOp(OperationExpression::OpAdd, Expr::MakeId(X86_Reg_Sp, &m_rCpuInfo), Expr::MakeConst(IntType(16, ParamNr * 2))));
  case X86_Bit_32: return Expr::MakeMem(32, Expr::MakeId(X86_Reg_Ss, &m_rCpuInfo),
    Expr::MakeBinOp(OperationExpression::OpAdd, Expr::MakeId(X86_Reg_Esp, &m_rCpuInfo), Expr::MakeConst(IntType(32, ParamNr * 4))));
  default: return nullptr;
  }
}

Expression::SPType CdeclCallingConvention::EmitReturnFromFunction(u16 ParamNo) const
{
  // TODO(wisk)
  return nullptr;
}

Expression::SPType CdeclCallingConvention::EmitReturnValueFromFunction(u16 ParamNo, IntType const& rRetVal) const
{
  // TODO(wisk)
  return nullptr;
}

CallingConvention::RegisterType CdeclCallingConvention::GetRegisterType(u32 Id) const
{
  switch (m_Mode)
  {
  case X86_Bit_16:
    switch (Id)
    {
    case X86_Reg_Ax: case X86_Reg_Cx: case X86_Reg_Dx:
      return CallingConvention::VolatileRegister;
    case X86_Reg_Bx: case X86_Reg_Si: case X86_Reg_Di:
    case X86_Reg_Sp: case X86_Reg_Bp: return CallingConvention::NonVolatileRegister;
    default: return CallingConvention::UnknownRegister;
    }
  case X86_Bit_32:
    switch (Id)
    {
    case X86_Reg_Eax: case X86_Reg_Ecx: case X86_Reg_Edx:
      return CallingConvention::VolatileRegister;
    case X86_Reg_Ebx: case X86_Reg_Esi: case X86_Reg_Edi:
    case X86_Reg_Esp: case X86_Reg_Ebp:
      return CallingConvention::NonVolatileRegister;
    default: return CallingConvention::UnknownRegister;
    }
  default: return CallingConvention::UnknownRegister;
  }
}

bool CdeclCallingConvention::AnalyzeArgument(Expression::SPType spExpr, u16& rArgNr, CallingConvention::ArgumentType& rArgType) const
{
  // TODO(wisk)
  return false;
}

CallingConvention::StackCleanerType CdeclCallingConvention::StackCleanupBy(void) const
{
  return StackCleanedByCaller;
}

// stdcall ////////////////////////////////////////////////////////////////////

bool StdCallCallingConvention::GetIntParameter(CpuContext const* pCpuCtxt, MemoryContext const* pMemCtxt, u16 ParamNr, IntType& rParamValue) const
{
  switch (m_Mode)
  {
  case X86_Bit_16:
  {
    u16 StkSeg;
    u16 StkOff;
    if (!pCpuCtxt->ReadRegister(X86_Reg_Ss, StkSeg));
    if (!pCpuCtxt->ReadRegister(X86_Reg_Sp, StkOff))
      return false;
    u64 StkLinAddr;
    if (!pCpuCtxt->Translate(Address(StkSeg, StkOff), StkLinAddr))
      return false;
    u16 ParamVal;
    if (!pMemCtxt->ReadMemory(StkLinAddr + (ParamNr + 1) * 2, ParamVal))
      return false;
    rParamValue = IntType(ParamVal);
    return true;
  }

  case X86_Bit_32:
  {
    u16 StkSeg;
    u32 StkOff;
    if (!pCpuCtxt->ReadRegister(X86_Reg_Ss, StkSeg));
    if (!pCpuCtxt->ReadRegister(X86_Reg_Esp, StkOff))
      return false;
    u64 StkLinAddr;
    if (!pCpuCtxt->Translate(Address(StkSeg, StkOff), StkLinAddr))
      return false;
    u32 ParamVal;
    if (!pMemCtxt->ReadMemory(StkLinAddr + (ParamNr + 1) * 4, ParamVal))
      return false;
    rParamValue = IntType(ParamVal);
    return true;
  }

  default:
    return false;
  }
}

bool StdCallCallingConvention::ReturnFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo) const
{
  // TODO(wisk)
  return false;
}

bool StdCallCallingConvention::ReturnValueFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo, IntType const& rRetVal) const
{
  // TODO(wisk)
  return false;
}

Expression::SPType StdCallCallingConvention::EmitGetIntParameter(u16 ParamNr) const
{
  switch (m_Mode)
  {
  case X86_Bit_16: return Expr::MakeMem(16, Expr::MakeId(X86_Reg_Ss, &m_rCpuInfo),
    Expr::MakeBinOp(OperationExpression::OpAdd, Expr::MakeId(X86_Reg_Sp, &m_rCpuInfo), Expr::MakeConst(IntType(16, ParamNr * 2))));
  case X86_Bit_32: return Expr::MakeMem(32, Expr::MakeId(X86_Reg_Ss, &m_rCpuInfo),
    Expr::MakeBinOp(OperationExpression::OpAdd, Expr::MakeId(X86_Reg_Esp, &m_rCpuInfo), Expr::MakeConst(IntType(32, ParamNr * 4))));
  default: return nullptr;
  }
}

Expression::SPType StdCallCallingConvention::EmitReturnFromFunction(u16 ParamNo) const
{
  // TODO(wisk)
  return nullptr;
}

Expression::SPType StdCallCallingConvention::EmitReturnValueFromFunction(u16 ParamNo, IntType const& rRetVal) const
{
  // TODO(wisk)
  return nullptr;
}

CallingConvention::RegisterType StdCallCallingConvention::GetRegisterType(u32 Id) const
{
  switch (m_Mode)
  {
  case X86_Bit_16:
    switch (Id)
    {
    case X86_Reg_Ax: case X86_Reg_Cx: case X86_Reg_Dx:
      return CallingConvention::VolatileRegister;
    case X86_Reg_Bx: case X86_Reg_Si: case X86_Reg_Di:
    case X86_Reg_Sp: case X86_Reg_Bp: return CallingConvention::NonVolatileRegister;
    default: return CallingConvention::UnknownRegister;
    }
  case X86_Bit_32:
    switch (Id)
    {
    case X86_Reg_Eax: case X86_Reg_Ecx: case X86_Reg_Edx:
      return CallingConvention::VolatileRegister;
    case X86_Reg_Ebx: case X86_Reg_Esi: case X86_Reg_Edi:
    case X86_Reg_Esp: case X86_Reg_Ebp:
      return CallingConvention::NonVolatileRegister;
    default: return CallingConvention::UnknownRegister;
    }
  default: return CallingConvention::UnknownRegister;
  }
}

bool StdCallCallingConvention::AnalyzeArgument(Expression::SPType spExpr, u16& rArgNr, CallingConvention::ArgumentType& rArgType) const
{
  // TODO(wisk)
  return false;
}

CallingConvention::StackCleanerType StdCallCallingConvention::StackCleanupBy(void) const
{
  return StackCleanedByCallee;
}

// Microsoft x64 //////////////////////////////////////////////////////////////

bool MsX64CallingConvention::GetIntParameter(CpuContext const* pCpuCtxt, MemoryContext const* pMemCtxt, u16 ParamNr, IntType& rParamValue) const
{
  // TODO(wisk)
  return false;
}

bool MsX64CallingConvention::ReturnFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo) const
{
  // TODO(wisk)
  return false;
}

bool MsX64CallingConvention::ReturnValueFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo, IntType const& rRetVal) const
{
  // TODO(wisk)
  return false;
}

Expression::SPType MsX64CallingConvention::EmitGetIntParameter(u16 ParamNr) const
{
  u32 Reg = 0;
  switch (ParamNr)
  {
  case 0: Reg = X86_Reg_Rcx; break;
  case 1: Reg = X86_Reg_Rdx; break;
  case 2: Reg = X86_Reg_R8; break;
  case 3: Reg = X86_Reg_R9; break;
  default: return nullptr; // TODO(wisk): handle stack based parameter
  }
  return Expr::MakeId(Reg, &m_rCpuInfo);
}

Expression::SPType MsX64CallingConvention::EmitReturnFromFunction(u16 ParamNo) const
{
  // TODO(wisk)
  return nullptr;
}

Expression::SPType MsX64CallingConvention::EmitReturnValueFromFunction(u16 ParamNo, IntType const& rRetVal) const
{
  // TODO(wisk)
  return nullptr;
}

// src: https://msdn.microsoft.com/en-us/library/6t169e9c.aspx
CallingConvention::RegisterType MsX64CallingConvention::GetRegisterType(u32 Id) const
{
  switch (Id)
  {
  case X86_Reg_Rax: case X86_Reg_Rcx: case X86_Reg_Rdx: case X86_Reg_R8: case X86_Reg_R9: case X86_Reg_R10: case X86_Reg_R11:
    return CallingConvention::VolatileRegister;
  case X86_Reg_Rbx: case X86_Reg_Rbp: case X86_Reg_Rdi: case X86_Reg_Rsi: case X86_Reg_Rsp: case X86_Reg_R12: case X86_Reg_R13: case X86_Reg_R14: case X86_Reg_R15:
    return CallingConvention::NonVolatileRegister;
  default:
    return CallingConvention::UnknownRegister;
  }
}

bool MsX64CallingConvention::AnalyzeArgument(Expression::SPType spExpr, u16& rArgNr, CallingConvention::ArgumentType& rArgType) const
{
  // TODO(wisk)
  return false;
}

CallingConvention::StackCleanerType MsX64CallingConvention::StackCleanupBy(void) const
{
  return StackCleanedByCallee;
}

// System V ///////////////////////////////////////////////////////////////////

bool SystemVCallingConvention::GetIntParameter(CpuContext const* pCpuCtxt, MemoryContext const* pMemCtxt, u16 ParamNr, IntType& rParamValue) const
{
  // TODO(wisk)
  return false;
}

bool SystemVCallingConvention::ReturnFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo) const
{
  // TODO(wisk)
  return false;
}

bool SystemVCallingConvention::ReturnValueFromFunction(CpuContext* pCpuCtxt, MemoryContext* pMemCtxt, u16 ParamNo, IntType const& rRetVal) const
{
  // TODO(wisk)
  return false;
}

Expression::SPType SystemVCallingConvention::EmitGetIntParameter(u16 ParamNr) const
{
  u32 Reg = 0;
  switch (ParamNr)
  {
  case 0: Reg = X86_Reg_Rcx; break;
  case 1: Reg = X86_Reg_Rdx; break;
  case 2: Reg = X86_Reg_R8; break;
  case 3: Reg = X86_Reg_R9; break;
  default: return nullptr; // TODO(wisk): handle stack based parameter
  }
  return Expr::MakeId(Reg, &m_rCpuInfo);
}

Expression::SPType SystemVCallingConvention::EmitReturnFromFunction(u16 ParamNo) const
{
  // TODO(wisk)
  return nullptr;
}

Expression::SPType SystemVCallingConvention::EmitReturnValueFromFunction(u16 ParamNo, IntType const& rRetVal) const
{
  // TODO(wisk)
  return nullptr;
}

CallingConvention::RegisterType SystemVCallingConvention::GetRegisterType(u32 Id) const
{
  switch (Id)
  {
  case X86_Reg_Rax: case X86_Reg_Rcx: case X86_Reg_Rdx: case X86_Reg_Rdi: case X86_Reg_Rsi: case X86_Reg_R8: case X86_Reg_R9: case X86_Reg_R10: case X86_Reg_R11:
    return CallingConvention::VolatileRegister;
  case X86_Reg_Rbx: case X86_Reg_Rbp: case X86_Reg_Rsp: case X86_Reg_R12: case X86_Reg_R13: case X86_Reg_R14: case X86_Reg_R15:
    return CallingConvention::NonVolatileRegister;
  default:
    return CallingConvention::UnknownRegister;
  }
}

bool SystemVCallingConvention::AnalyzeArgument(Expression::SPType spExpr, u16& rArgNr, CallingConvention::ArgumentType& rArgType) const
{
  // TODO(wisk)
  return false;
}

CallingConvention::StackCleanerType SystemVCallingConvention::StackCleanupBy(void) const
{
  return StackCleanedByCallee;
}