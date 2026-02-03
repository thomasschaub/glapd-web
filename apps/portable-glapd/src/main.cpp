#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <zip.h>

#include "bowtie.h"
#include "glapd.h"
#include "par.h"
#include "signals.h"

#if EMSCRIPTEN
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace fs = std::filesystem;

const char* workingDirectory = "/tmp";
const char* bowtieIndexPath = "/tmp/index";

static std::string s_parPath;

[[noreturn]]
static void die(const char* fmt, ...) {
    // Append \n
    std::string fmtWithNewline = fmt;
    fmtWithNewline += "\n";

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmtWithNewline.c_str(), args);
    va_end(args);

    fflush(stderr);

    std::exit(1);
}

static std::string getParPath() {
    // For dev builds, use external/glapd/GLAPD/Par
    const char* candidate = "external/glapd/GLAPD/Par";
    if (fs::is_directory(candidate))
        return candidate;

    die("Could not determine par path");
}

enum class BackgroundMode {
    none,
    automatic,
    fromFile,
};

static std::string toString(BackgroundMode b) {
    switch (b) {
        case BackgroundMode::none: return "none";
        case BackgroundMode::automatic: return "automatic";
        case BackgroundMode::fromFile: return "fromFile";
        default: throw std::invalid_argument("Invalid BackgroundMode");
    }
}

static BackgroundMode toBackgroundMode(const std::string_view s) {
    if (s == "none")
        return BackgroundMode::none;
    else if (s == "automatic")
        return BackgroundMode::automatic;
    else if (s == "fromFile")
        return BackgroundMode::fromFile;
    else
        throw std::invalid_argument("Cannot convert to BackgroundMode");
}

struct Args {
    std::string indexPath = ""; // path to .fa file, used to build Bowtie index

    std::string refPath = "";

    std::string targetListPath = "";
    unsigned maxNumMismatchesInTarget = 0;

    BackgroundMode backgroundMode = BackgroundMode::automatic;
    std::string backgroundListPath = "";
    unsigned maxNumMismatchesInBackground = 2;

    bool includeLoopPrimers = false;
    unsigned numPrimersToGenerate = 10;

    unsigned numThreads = 1;
};

std::string renderArgs(const Args& args) {
    return std::format(
        "maxNumMismatchesInTarget: {}\n"
        "backgroundMode: {}\n"
        "maxNumMismatchesInBackground: {}\n"
        "includeLoopPrimers: {}\n"
        "numPrimersToGenerate: {}\n"
        "numThreads: {}\n",
        args.maxNumMismatchesInTarget,
        toString(args.backgroundMode),
        args.maxNumMismatchesInBackground,
        args.includeLoopPrimers,
        args.numPrimersToGenerate,
        args.numThreads);
}

unsigned parseUintArg(const char* name, const char* value) {
    try {
        return std::stoul(value);
    } catch (const std::exception&) {
        die("Illegal value for --%s", name);
    }
}

Args parseArgs(int argc, char* argv[]) {
    Args args;

    for (int i = 1; i < argc; i++)
    {
        const std::string_view arg = argv[i];
        if (arg == "--index") {
            if (i + 1 >= argc)
                die("Missing argument value for --index");
            const char* val = argv[++i];
            args.indexPath = val;
        } else if (arg == "--ref") {
            if (i + 1 >= argc)
                die("Missing argument value --ref");
            const char* val = argv[++i];
            args.refPath = val;
        } else if (arg == "--target") {
            if (i + 1 >= argc)
                die("Missing argument value --target");
            const char* val = argv[++i];
            args.targetListPath = val;
        } else if (arg == "--maxNumMismatchesInTarget") {
            if (i + 1 >= argc)
                die("Missing argument value --maxNumMismatchesInTarget");
            const char* val = argv[++i];
            args.maxNumMismatchesInTarget = parseUintArg("maxNumMismatchesInTarget", val);
        } else if (arg == "--backgroundMode") {
            if (i + 1 >= argc)
                die("Missing argument value --backgroundMode");
            const char* val = argv[++i];
            try {
                args.backgroundMode = toBackgroundMode(val);
            } catch (const std::exception&) {
                die("Illegal value for --backgroundMode");
            }
        } else if (arg == "--backgroundListPath") {
            if (i + 1 >= argc)
                die("Missing argument value --backgroundListPath");
            const char* val = argv[++i];
            args.backgroundListPath = val;
        } else if (arg == "--maxNumMismatchesInBackground") {
            if (i + 1 >= argc)
                die("Missing argument value --maxNumMismatchesInBackground");
            const char* val = argv[++i];
            args.maxNumMismatchesInBackground = parseUintArg("maxNumMismatchesInBackground", val);
        } else if (arg == "--includeLoopPrimers") {
            args.includeLoopPrimers = true;
        } else if (arg == "--numPrimersToGenerate") {
            if (i + 1 >= argc)
                die("Missing argument value --numPrimersToGenerate");
            const char* val = argv[++i];
            args.numPrimersToGenerate = parseUintArg("numPrimersToGenerate", val);
        } else if (arg == "--numThreads") {
            if (i + 1 >= argc)
                die("Missing argument value --numThreads");
            const char* val = argv[++i];
            args.numThreads = parseUintArg("numThreads", val);
        } else {
            die("Unknown argument: %s", arg.data());
        }
    }

    return args;
}

void buildBowtieIndex(const Args& args) {
    notify_about_to_start_phase("buildBowtieIndex");

    std::vector<const char*> bowtieArgs {
        "bowtie-build", // program name
        args.indexPath.c_str(),
        bowtieIndexPath,
    };

    bowtie_build(bowtieArgs.size(), bowtieArgs.data());
}

void generateSingleRegionPrimers(const Args& args) {
    notify_about_to_start_phase("generateSingleRegionPrimers");

    std::cout << "Generating single region primers" << std::endl;

    std::vector<const char*> glapdArgs{
        "Single",
        "-in", args.refPath.c_str(),
        "-out", "NAME",
        "-dir", workingDirectory,
        "-par", s_parPath.c_str(),
    };

    if (args.includeLoopPrimers)
        glapdArgs.push_back("-loop");

    glapd_single_main(glapdArgs.size(), glapdArgs.data());
}

void alignSingleRegionPrimers(const Args& args) {
    notify_about_to_start_phase("alignSingleRegionPrimers");

    std::cout << "Aligning single region primers" << std::endl;

    const std::string misCStr = std::to_string(args.maxNumMismatchesInTarget);
    const std::string misSStr = std::to_string(args.maxNumMismatchesInBackground);
    const std::string numThreadsStr = std::to_string(args.numThreads);

    std::vector<const char*> parplArgs{
        "", // program name, unused
        "--in", "NAME",
        "--ref", args.refPath.c_str(),
        "--dir", workingDirectory,
        "--index", bowtieIndexPath,
        "--mis_c", misCStr.c_str(),
        "--mis_s", misSStr.c_str(),
        "--threads", numThreadsStr.c_str(),
    };

    if (args.includeLoopPrimers)
        parplArgs.push_back("--loop");

    if (!args.targetListPath.empty())
    {
        parplArgs.push_back("--common");
        parplArgs.push_back(args.targetListPath.c_str());
    }

    switch (args.backgroundMode) {
    case BackgroundMode::none:
        break; // nothing to do
    case BackgroundMode::automatic:
        parplArgs.push_back("--left");
        break;
    case BackgroundMode::fromFile:
        parplArgs.push_back("--specific");
        parplArgs.push_back(args.backgroundListPath.c_str());
        break;
    }

    parpl_main(parplArgs.size(), parplArgs.data());
}

void generateLampPrimerSets(const Args& args) {
    notify_about_to_start_phase("generateLampPrimerSets");

    std::cout << "Generating LAMP primer sets" << std::endl;

    const std::string numPrimersToGenerateStr = std::to_string(args.numPrimersToGenerate);

    std::vector<const char*> glapdArgs{
        "", // program name, unused
        "-in", "NAME",
        "-ref", args.refPath.c_str(),
        "-dir", workingDirectory,
        "-out", "success.txt",
        "-num", numPrimersToGenerateStr.c_str(),
        "-par", s_parPath.c_str(),
    };

    if (!args.targetListPath.empty())
        glapdArgs.push_back("-common");

    if (args.backgroundMode != BackgroundMode::none)
        glapdArgs.push_back("-specific");

    if (args.includeLoopPrimers)
        glapdArgs.push_back("-loop");

    glapd_lamp_main(glapdArgs.size(), glapdArgs.data());
}

static void createFileInZipFromString(zipFile zip, const std::filesystem::path& dst, const std::string& contents)
{
    zipOpenNewFileInZip(zip, dst.string().c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    zipWriteInFileInZip(zip, contents.c_str(), contents.size());
    zipCloseFileInZip(zip);
}

static void copyFileIntoZip(zipFile zip, const std::filesystem::path& src, const std::filesystem::path& dst)
{
    zipOpenNewFileInZip(zip, dst.string().c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

    // Read file in 1MB chunks
    std::ifstream in(src);
    const size_t bufSize = 1024 * 1024; // 1 MB
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(bufSize);
    while (in) {
        in.read(buf.get(), bufSize);
        const size_t numRead = in.gcount();
        zipWriteInFileInZip(zip, buf.get(), numRead);
    }

    zipCloseFileInZip(zip);
}

static void createWorkspaceZip(const Args& args)
{
    notify_about_to_start_phase("createWorkspaceZip");

    zipFile zip = zipOpen("workspace.zip", APPEND_STATUS_CREATE);

    // Inputs
    createFileInZipFromString(zip, "inputs/options.txt", renderArgs(args));
    copyFileIntoZip(zip, args.indexPath, "inputs/index.fasta");
    copyFileIntoZip(zip, args.refPath, "inputs/ref.fasta");
    copyFileIntoZip(zip, args.targetListPath, "inputs/target.fasta");
    if (args.backgroundMode == BackgroundMode::fromFile)
        copyFileIntoZip(zip, args.backgroundListPath, "inputs/background.fasta");

    // Outputs

    // Bowtie Index
    copyFileIntoZip(zip, "/tmp/index.1.ebwt", "outputs/index/index.1.ebwt");
    copyFileIntoZip(zip, "/tmp/index.2.ebwt", "outputs/index/index.2.ebwt");
    copyFileIntoZip(zip, "/tmp/index.3.ebwt", "outputs/index/index.3.ebwt");
    copyFileIntoZip(zip, "/tmp/index.4.ebwt", "outputs/index/index.4.ebwt");
    copyFileIntoZip(zip, "/tmp/index.rev.1.ebwt", "outputs/index/index.rev.1.ebwt");
    copyFileIntoZip(zip, "/tmp/index.rev.2.ebwt", "outputs/index/index.rev.2.ebwt");

    // Inner
    copyFileIntoZip(zip, "/tmp/Inner/NAME", "outputs/Inner/NAME");
    copyFileIntoZip(zip, "/tmp/Inner/NAME_Inner.bowtie", "outputs/Inner/NAME_Inner.bowtie");
    copyFileIntoZip(zip, "/tmp/Inner/NAME-common_list.txt", "outputs/Inner/NAME-common_list.txt");
    copyFileIntoZip(zip, "/tmp/Inner/NAME-common.txt", "outputs/Inner/NAME-common.txt");
    copyFileIntoZip(zip, "/tmp/Inner/NAME-specific.txt", "outputs/Inner/NAME-specific.txt");

    // Outer
    copyFileIntoZip(zip, "/tmp/Outer/NAME", "outputs/Outer/NAME");
    copyFileIntoZip(zip, "/tmp/Outer/NAME_Outer.bowtie", "outputs/Outer/NAME_Outer.bowtie");
    copyFileIntoZip(zip, "/tmp/Outer/NAME-common.txt", "outputs/Outer/NAME-common.txt");
    copyFileIntoZip(zip, "/tmp/Outer/NAME-specific.txt", "outputs/Outer/NAME-specific.txt");

    // Loop
    if (args.includeLoopPrimers)
    {
        copyFileIntoZip(zip, "/tmp/Loop/NAME", "outputs/Loop/NAME");
        copyFileIntoZip(zip, "/tmp/Loop/NAME_Loop.bowtie", "outputs/Loop/NAME_Loop.bowtie");
        copyFileIntoZip(zip, "/tmp/Loop/NAME-common.txt", "outputs/Loop/NAME-common.txt");
        copyFileIntoZip(zip, "/tmp/Loop/NAME-specific.txt", "outputs/Loop/NAME-specific.txt");
    }

    copyFileIntoZip(zip, "success.txt", "outputs/success.txt");

    // Logs?!

    zipClose(zip, nullptr);
}

static void runGlapd(const Args& args)
{
    buildBowtieIndex(args);
    generateSingleRegionPrimers(args);
    alignSingleRegionPrimers(args);
    generateLampPrimerSets(args);
    createWorkspaceZip(args);
}

static bool isValidFile(const std::string& path) {
    return !path.empty() && fs::is_regular_file(path);
}

int main(int argc, char* argv[])
{
    try {
        s_parPath = getParPath();

        const Args args = parseArgs(argc, argv);

        // Verify arguments
        if (!isValidFile(args.indexPath))
            die("Invalid index path");
        if (!isValidFile(args.refPath))
            die("Invalid ref path");
        if (!args.targetListPath.empty() && !isValidFile(args.targetListPath))
            die("Invalid target list path");
        if (args.backgroundMode != BackgroundMode::fromFile) {
            if (!args.backgroundListPath.empty())
                die("--backgroundListPath set, but --backgroundMode is not fromFile");
        } else {
            if (!isValidFile(args.backgroundListPath))
                die("Invalid background list path");
        }

        // Run GLAPD
        const auto startTime = std::chrono::steady_clock::now();
        runGlapd(args);
        const auto endTime = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        std::printf("Done. Took %lli seconds in total\n", duration.count());

        return 0;
    } catch (const std::exception& e) {
        std::printf("Unhandled exception: %s\n", e.what());
        return 1;
    }
}
