// generated by codegen/codegen.py
import codeql.swift.elements.decl.AbstractStorageDecl
import codeql.swift.elements.type.Type

class VarDeclBase extends @var_decl, AbstractStorageDecl {

  string getName() {
    var_decls(this, result, _)
  }

  Type getType() {
    exists(Type x |
      var_decls(this, _, x)
      and
      result = x.resolve())
  }
}
