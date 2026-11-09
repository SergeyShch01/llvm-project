//===-- Lower/HostAssociations.h --------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef FORTRAN_LOWER_HOSTASSOCIATIONS_H
#define FORTRAN_LOWER_HOSTASSOCIATIONS_H

#include "mlir/IR/Types.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include <optional>

namespace Fortran {
namespace semantics {
class Symbol;
class Scope;
} // namespace semantics

namespace lower {
class AbstractConverter;
class SymMap;
namespace pft {
struct Program;
struct Variable;
} // namespace pft

/// Internal procedures in Fortran may access variables declared in the host
/// procedure directly. We bundle these variables together in a tuple and pass
/// them as an extra argument.
class HostAssociations {
public:
  /// Returns true iff there are no host associations.
  bool empty() const { return tupleSymbols.empty() && globalSymbols.empty(); }

  /// Returns true iff there are host associations that are conveyed through
  /// an extra tuple argument.
  bool hasTupleAssociations() const { return !tupleSymbols.empty(); }

  /// Classify host variables as globals or tuple entries using Program.
  /// Cached variable records are owned by Program.
  void addSymbolsToBind(
      const llvm::SetVector<const Fortran::semantics::Symbol *> &symbols,
      const Fortran::semantics::Scope &hostScope, pft::Program &program);

  /// Return cached MAIN variables when a captured scalar is selected for static
  /// storage, or std::nullopt otherwise. Program owns the list and must outlive
  /// the returned view.
  std::optional<llvm::ArrayRef<pft::Variable>> getVariables() const {
    return variables;
  }

  /// Code gen the FIR for the local bindings for the host associated symbols
  /// for the host (parent) procedure using `builder`.
  void hostProcedureBindings(AbstractConverter &converter, SymMap &symMap);

  /// Code gen the FIR for the local bindings for the host associated symbols
  /// for an internal (child) procedure using `builder`.
  void internalProcedureBindings(AbstractConverter &converter, SymMap &symMap);

  /// Return the type of the extra argument to add to each internal procedure.
  mlir::Type getArgumentType(AbstractConverter &convert);

  /// Is \p symbol host associated ?
  bool isAssociated(const Fortran::semantics::Symbol &symbol) const {
    return tupleSymbols.contains(&symbol) || globalSymbols.contains(&symbol);
  }

  /// Is the canonical symbol captured as a global instead of a tuple entry?
  bool isAssociatedGlobal(const Fortran::semantics::Symbol &symbol) const {
    return globalSymbols.contains(&symbol);
  }

private:
  /// Canonical vector of host associated local symbols.
  llvm::SetVector<const Fortran::semantics::Symbol *> tupleSymbols;

  /// Canonical vector of host associated global symbols.
  llvm::SetVector<const Fortran::semantics::Symbol *> globalSymbols;

  /// Ordered records for captured globals and the storage backing their
  /// aliases. The records refer to Program's cached lists and remain valid
  /// until it is destroyed.
  llvm::SmallVector<const pft::Variable *> globalBindings;
  std::optional<llvm::ArrayRef<pft::Variable>> variables;

  /// The type of the extra argument to be added to each internal procedure.
  mlir::Type argType;

  /// Scope of the parent procedure if addSymbolsToBind was called.
  const Fortran::semantics::Scope *hostScope;
};
} // namespace lower
} // namespace Fortran

#endif // FORTRAN_LOWER_HOSTASSOCIATIONS_H
