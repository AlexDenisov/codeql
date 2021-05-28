import ql
import codeql_ql.ast.internal.AstNodes

private class TScope =
  TClass or TAggregate or TExprAggregate or TQuantifier or TSelect or TPredicate or TNewTypeBranch;

/** A variable scope. */
class VariableScope extends TScope, AstNode {
  /** Gets the outer scope, if any. */
  VariableScope getOuterScope() { result = scopeOf(this) }

  /** Gets a variable declared directly in this scope. */
  VarDef getADefinition() { result.getParent() = this }

  /** Holds if this scope contains variable `decl`, either directly or inherited. */
  predicate containsVar(VarDef decl) {
    not this instanceof Class and
    decl = this.getADefinition()
    or
    decl = this.(Select).getExpr(_).(AsExpr)
    or
    decl = this.(Aggregate).getExpr(_).(AsExpr)
    or
    decl = this.(ExprAggregate).getExpr(_).(AsExpr)
    or
    this.getOuterScope().containsVar(decl) and
    not this.getADefinition().getName() = decl.getName()
  }

  /** Holds if this scope contains field `decl`, either directly or inherited. */
  predicate containsField(VarDef decl) {
    decl = this.(Class).getAField()
    or
    this.getOuterScope().containsField(decl) and
    not this.getADefinition().getName() = decl.getName()
    or
    exists(VariableScope sup |
      sup = this.(Class).getASuperType().getResolvedType().(ClassType).getDeclaration() and
      sup.containsField(decl) and
      not this.(Class).getAField().getName() = decl.getName()
    )
  }
}

private AstNode parent(AstNode child) {
  result = child.getParent() and
  not child instanceof VariableScope
}

VariableScope scopeOf(AstNode n) { result = parent*(n.getParent()) }

private string getName(Identifier i) {
  exists(Generated::Variable v |
    i = TIdentifier(v) and
    result = v.getChild().(Generated::VarName).getChild().getValue()
  )
}

predicate resolveVariable(Identifier i, VarDef decl) {
  scopeOf(i).containsVar(decl) and
  decl.getName() = getName(i)
}

predicate resolveField(Identifier i, VarDef decl) {
  scopeOf(i).containsField(decl) and
  decl.getName() = getName(i)
}

module VarConsistency {
  query predicate multipleVarDefs(VarAccess v, VarDef decl) {
    decl = v.getDeclaration() and
    strictcount(v.getDeclaration()) > 1
  }

  query predicate multipleFieldDefs(FieldAccess f, VarDef decl) {
    decl = f.getDeclaration() and
    strictcount(f.getDeclaration()) > 1
  }
}
