// generated by codegen/codegen.py
import codeql.swift.elements.expr.Expr
import codeql.swift.elements.stmt.Stmt

class ThrowStmtBase extends @throw_stmt, Stmt {
  override string toString() { result = "ThrowStmt" }

  Expr getSubExpr() {
    exists(Expr x |
      throw_stmts(this, x)
      and
      result = x.resolve())
  }
}
