#include "swift/extractor/infra/TargetFile.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

namespace codeql {
namespace {
[[noreturn]] void error(const char* action, const std::string& arg, std::error_code ec) {
  std::cerr << "Unable to " << action << ": " << arg << " (" << ec.message() << ")\n";
  std::abort();
}

[[noreturn]] void error(const char* action, const std::string& arg) {
  error(action, arg, {errno, std::system_category()});
}

void ensureParentDir(const std::string& path) {
  auto parent = llvm::sys::path::parent_path(path);
  if (auto ec = llvm::sys::fs::create_directories(parent)) {
    error("create directory", parent.str(), ec);
  }
}

std::string initPath(std::string_view target, std::string_view dir) {
  std::string ret{dir};
  assert(!target.empty() && "target must be a non-empty path");
  if (target[0] != '/') {
    ret += '/';
  }
  ret.append(target);
  ensureParentDir(ret);
  return ret;
}
}  // namespace

TargetFile::TargetFile(std::string_view target,
                       std::string_view targetDir,
                       std::string_view workingDir)
    : workingPath{initPath(target, workingDir)}, targetPath{initPath(target, targetDir)} {
  errno = 0;
  // since C++17 "x" mode opens with O_EXCL (fails if file already exists)
  if (auto f = std::fopen(targetPath.c_str(), "wx")) {
    std::fclose(f);
    out.open(workingPath);
    checkOutput("open file for writing");
  } else {
    if (errno != EEXIST) {
      error("open file for writing", targetPath);
    }
    // else we just lost the race, do nothing (good() will return false to signal this)
  }
}

bool TargetFile::good() const {
  return out && out.is_open();
}

void TargetFile::commit() {
  assert(good());
  out.close();
  errno = 0;
  if (std::rename(workingPath.c_str(), targetPath.c_str()) != 0) {
    error("rename file", targetPath);
  }
}

void TargetFile::checkOutput(const char* action) {
  if (!out) {
    error(action, workingPath);
  }
}

}  // namespace codeql
