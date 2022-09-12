#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include <swift/Basic/LLVMInitialize.h>
#include <swift/FrontendTool/FrontendTool.h>

#include "SwiftExtractor.h"
#include "SwiftOutputRewrite.h"
#include "fishhook/fishhook.h"
#include <sys/stat.h>

using namespace std::string_literals;

// This is part of the swiftFrontendTool interface, we hook into the
// compilation pipeline and extract files after the Swift frontend performed
// semantic analysis
class Observer : public swift::FrontendObserver {
 public:
  explicit Observer(const codeql::SwiftExtractorConfiguration& config,
                    std::vector<std::string>& outputs)
      : config{config}, outputs{outputs} {}

  void parsedArgs(swift::CompilerInvocation& invocation) override {
    /* auto& overlays = invocation.getSearchPathOptions().VFSOverlayFiles; */
    /* auto vfsFiles = codeql::collectVFSFiles(config); */
    /* for (auto& vfsFile : vfsFiles) { */
    /*   overlays.push_back(vfsFile); */
    /* } */
    outputs = invocation.getFrontendOptions().InputsAndOutputs.copyOutputFilenames();
  }

  void performedSemanticAnalysis(swift::CompilerInstance& compiler) override {
    codeql::extractSwiftFiles(config, compiler);
  }

 private:
  const codeql::SwiftExtractorConfiguration& config;
  std::vector<std::string>& outputs;
};

static std::string getenv_or(const char* envvar, const std::string& def) {
  if (const char* var = getenv(envvar)) {
    return var;
  }
  return def;
}

static std::string tempFolder;

void trace_open(const char* path, int flags, int fd) {
  fprintf(stderr, "path: %s, fd = %d, flags (%d): ", path, fd, flags);
  if ((flags & O_RDONLY)) {
    fprintf(stderr, "O_RDONLY ");
  }
  if ((flags & O_WRONLY)) {
    fprintf(stderr, "O_WRONLY ");
  }
  if ((flags & O_RDWR)) {
    fprintf(stderr, "O_RDWR ");
  }
  if ((flags & O_NONBLOCK)) {
    fprintf(stderr, "O_NONBLOCK ");
  }
  if ((flags & O_APPEND)) {
    fprintf(stderr, "O_APPEND ");
  }
  if ((flags & O_CREAT)) {
    fprintf(stderr, "O_CREAT ");
  }
  if ((flags & O_TRUNC)) {
    fprintf(stderr, "O_TRUNC ");
  }
  if ((flags & O_EXCL)) {
    fprintf(stderr, "O_EXCL ");
  }
  if ((flags & O_SHLOCK)) {
    fprintf(stderr, "O_SHLOCK ");
  }
  if ((flags & O_EXLOCK)) {
    fprintf(stderr, "O_EXLOCK ");
  }
  if ((flags & O_DIRECTORY)) {
    fprintf(stderr, "O_DIRECTORY ");
  }
  if ((flags & O_NOFOLLOW)) {
    fprintf(stderr, "O_NOFOLLOW ");
  }
  if ((flags & O_SYMLINK)) {
    fprintf(stderr, "O_SYMLINK ");
  }
  if ((flags & O_EVTONLY)) {
    fprintf(stderr, "O_EVTONLY ");
  }
  if ((flags & O_CLOEXEC)) {
    fprintf(stderr, "O_CLOEXEC ");
  }
  if ((flags & O_NOFOLLOW_ANY)) {
    fprintf(stderr, "O_NOFOLLOW_ANY ");
  }
  fprintf(stderr, "\n");
}

static int (*original_close)(int) = NULL;
int codeql_close(int fd) {
  /* fprintf(stderr, "closing fd %d\n", fd); */
  return original_close(fd);
}

std::string fileHash(const std::string& filename);

static int (*original_open)(const char*, int, ...) = NULL;
int codeql_open(const char* path, int oflag, ...) {
  int fd = 0;

  va_list ap = {0};
  mode_t mode = 0;
  if ((oflag & O_CREAT) != 0) {
    // mode only applies to O_CREAT
    va_start(ap, oflag);
    mode = va_arg(ap, int);
    va_end(ap);
  }

  std::string newPath(path);

  if (access(newPath.c_str(), F_OK) == 0) {
    auto hash = fileHash(newPath);
    auto hashedFile = tempFolder + "/" + hash;
    if (!hash.empty() && (access(hashedFile.c_str(), F_OK) == 0)) {
      newPath = hashedFile;
      fprintf(stderr, "Opening %s instead of %s\n", newPath.c_str(), path);
    }
  }

  fd = original_open(newPath.c_str(), oflag, mode);
  trace_open(path, oflag, fd);
  return fd;
}

std::string fileHash(const std::string& filename) {
  int fd = original_open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    perror("original_open");
    return {};
  }
  struct stat st {};
  fstat(fd, &st);
  auto maybeBuffer = llvm::MemoryBuffer::getOpenFile(fd, filename, st.st_size);
  if (!maybeBuffer) {
    llvm::errs() << maybeBuffer.getError().message() << "\n";
    original_close(fd);
    return {};
  }
  auto& buffer = maybeBuffer.get();
  llvm::MD5 hash;
  hash.update(buffer->getBuffer());
  llvm::MD5::MD5Result result;
  hash.final(result);
  original_close(fd);
  return result.digest().str().str();
}

int main(int argc, char** argv) {
  if (argc == 1) {
    // TODO: print usage
    return 1;
  }
  rebind_symbols((struct rebinding[2]){{"open", reinterpret_cast<void*>(codeql_open),
                                        reinterpret_cast<void**>(&original_open)},
                                       {"close", reinterpret_cast<void*>(codeql_close),
                                        reinterpret_cast<void**>(&original_close)}},
                 2);
  // Required by Swift/LLVM
  PROGRAM_START(argc, argv);
  INITIALIZE_LLVM();

  codeql::SwiftExtractorConfiguration configuration{};
  configuration.trapDir = getenv_or("CODEQL_EXTRACTOR_SWIFT_TRAP_DIR", ".");
  configuration.sourceArchiveDir = getenv_or("CODEQL_EXTRACTOR_SWIFT_SOURCE_ARCHIVE_DIR", ".");
  configuration.scratchDir = getenv_or("CODEQL_EXTRACTOR_SWIFT_SCRATCH_DIR", ".");

  configuration.frontendOptions.reserve(argc - 1);
  for (int i = 1; i < argc; i++) {
    configuration.frontendOptions.push_back(argv[i]);
  }
  configuration.patchedFrontendOptions = configuration.frontendOptions;

  auto remapping =
      codeql::rewriteOutputsInPlace(configuration, configuration.patchedFrontendOptions);
  codeql::ensureDirectoriesForNewPathsExist(remapping);
  /* codeql::storeRemappingForVFS(configuration, remapping); */
  codeql::lockOutputSwiftModuleTraps(configuration, remapping);

  auto originalOutputs = codeql::getOutputs(configuration.frontendOptions);
  auto patchedOutputs = codeql::getOutputs(configuration.patchedFrontendOptions);

  std::vector<const char*> args;
  for (auto& arg : configuration.patchedFrontendOptions) {
    args.push_back(arg.c_str());
  }

  std::vector<std::string> outputs;

  tempFolder = configuration.getTempArtifactDir();

  Observer observer(configuration, outputs);
  int frontend_rc = swift::performFrontend(args, "swift-extractor", (void*)main, &observer);

  llvm::errs() << "Original \n";
  for (auto& out : originalOutputs) {
    if (!llvm::StringRef(out).endswith(".swiftmodule")) {
      continue;
    }
    auto hash = fileHash(out);
    llvm::errs() << "  " << out << " -> " << hash << "\n";
    auto patchedOut = tempFolder + "/" + out;
    auto hashedOut = tempFolder + "/" + hash;
    if (!hash.empty() && (access(patchedOut.c_str(), F_OK) == 0)) {
      if (std::error_code ec =
              llvm::sys::fs::create_link(/* from */ patchedOut, /* to */ hashedOut)) {
        llvm::errs() << "Cannot remap file '" << patchedOut << "' -> '" << hashedOut
                     << "': " << ec.message() << "\n";
      }
    }
  }

  llvm::errs() << "Patched \n";
  for (auto& out : patchedOutputs) {
    llvm::errs() << "  " << out << " -> " << fileHash(out) << "\n";
  }

  return frontend_rc;
}
