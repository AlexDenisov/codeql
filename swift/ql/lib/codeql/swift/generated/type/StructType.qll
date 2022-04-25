// generated by codegen/codegen.py
import codeql.swift.elements.type.NominalType
import codeql.swift.elements.decl.StructDecl

class StructTypeBase extends @struct_type, NominalType {
  override string toString() { result = "StructType" }

  StructDecl getDecl() {
    exists(StructDecl x |
      struct_types(this, x)
      and
      result = x.resolve())
  }
}
