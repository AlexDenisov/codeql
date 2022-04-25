// generated by codegen/codegen.py
import codeql.swift.elements.stmt.CaseStmt
import codeql.swift.elements.expr.Expr
import codeql.swift.elements.stmt.LabeledStmt

class SwitchStmtBase extends @switch_stmt, LabeledStmt {
  override string toString() { result = "SwitchStmt" }

  Expr getExpr() {
    exists(Expr x |
      switch_stmts(this, x)
      and
      result = x.resolve())
  }

  CaseStmt getCase(int index) {
    exists(CaseStmt x |
      switch_stmt_cases(this, index, x)
      and
      result = x.resolve())
  }

  CaseStmt getACase() {
    result = getCase(_)
  }

  int getNumberOfCases() {
    result = count(getACase())
  }
}
