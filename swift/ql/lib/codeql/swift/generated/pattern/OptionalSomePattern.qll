// generated by codegen/codegen.py
import codeql.swift.elements.pattern.Pattern

class OptionalSomePatternBase extends @optional_some_pattern, Pattern {
  override string toString() { result = "OptionalSomePattern" }

  Pattern getSubPattern() {
    exists(Pattern x |
      optional_some_patterns(this, x)
      and
      result = x.resolve())
  }
}
