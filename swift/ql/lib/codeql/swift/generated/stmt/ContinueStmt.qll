// generated by codegen/codegen.py
import codeql.swift.elements.stmt.Stmt

class ContinueStmtBase extends @continue_stmt, Stmt {
  override string toString() { result = "ContinueStmt" }

  string getTargetName() {
    continue_stmt_target_names(this, result)
  }

  Stmt getTarget() {
    exists(Stmt x |
      continue_stmt_targets(this, x)
      and
      result = x.resolve())
  }
}
