// generated by codegen/codegen.py
import codeql.swift.elements.expr.Expr
import codeql.swift.elements.decl.ParamDecl

class DefaultArgumentExprBase extends @default_argument_expr, Expr {
  override string toString() { result = "DefaultArgumentExpr" }

  ParamDecl getParamDecl() {
    exists(ParamDecl x |
      default_argument_exprs(this, x, _)
      and
      result = x.resolve())
  }

  int getParamIndex() {
    default_argument_exprs(this, _, result)
  }

  Expr getCallerSideDefault() {
    exists(Expr x |
      default_argument_expr_caller_side_defaults(this, x)
      and
      result = x.resolve())
  }
}
