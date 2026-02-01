// A C++ program to replicate the functionality of par.pl

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <filesystem>
#include <regex>
#include <cstdlib>
#include <ctime>

#include <bowtie.h>

namespace fs = std::filesystem;

enum class PrimerType {
    inner,
    outer,
    loop,
};

std::string toString(PrimerType t) {
    switch (t)
    {
        case PrimerType::inner: return "Inner";
        case PrimerType::outer: return "Outer";
        case PrimerType::loop: return "Loop";
        default: throw std::domain_error("Unexpected PrimerType");
    }
}

struct Config {
    std::string prefix;
    std::string common_file;
    std::string special_file;
    std::string ref_file;
    std::string dir;
    std::string index;
    int mis_c = 0;
    int mis_s = 2;
    int threads = 1;
    bool left = false;
    bool loop = false;
};

void printUsage() {
    std::cout << "USAGE: ./lamp_primer_check [options]\n"
              << "  --in <single_primers_file>\n"
              << "  --ref <ref_genome>\n"
              << "  --common <genomes_list>\n"
              << "  [--specific <genomes_list>] [--left] [--loop]\n"
              << "  --bowtie <bowtie> --index <database>\n"
              << "  [--mis_c <0-3>] [--mis_s <0-3>] [--threads <int>]\n";
    exit(EXIT_FAILURE);
}

void parseArgs(int argc, const char* argv[], Config& cfg) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--in" && i + 1 < argc) cfg.prefix = argv[++i];
        else if (arg == "--ref" && i + 1 < argc) cfg.ref_file = argv[++i];
        else if (arg == "--dir" && i + 1 < argc) cfg.dir = argv[++i];
        else if (arg == "--common" && i + 1 < argc) cfg.common_file = argv[++i];
        else if (arg == "--specific" && i + 1 < argc) cfg.special_file = argv[++i];
        else if (arg == "--index" && i + 1 < argc) cfg.index = argv[++i];
        else if (arg == "--mis_c" && i + 1 < argc) cfg.mis_c = std::stoi(argv[++i]);
        else if (arg == "--mis_s" && i + 1 < argc) cfg.mis_s = std::stoi(argv[++i]);
        else if (arg == "--threads" && i + 1 < argc) cfg.threads = std::stoi(argv[++i]);
        else if (arg == "--left") cfg.left = true;
        else if (arg == "--loop") cfg.loop = true;
        else printUsage();
    }

    if (cfg.prefix.empty() || cfg.ref_file.empty() || cfg.index.empty()) {
        printUsage();
    }
    if (cfg.dir.empty()) cfg.dir = fs::current_path().string();
    if (cfg.mis_c < 0 || cfg.mis_c > 3 || cfg.mis_s < 0 || cfg.mis_s > 3 || cfg.mis_c > cfg.mis_s) {
        std::cerr << "Invalid mismatch parameters.\n";
        exit(EXIT_FAILURE);
    }
}

std::string readFastaSequence(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) throw std::runtime_error("Cannot open file: " + file_path);
    std::string line, sequence;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '>') continue;
        sequence += line;
    }
    return sequence;
}

std::unordered_map<std::string, unsigned> loadGenomeIds(const std::string& file_path, std::vector<std::string>& names) {
    std::unordered_map<std::string, unsigned> result;
    std::ifstream file(file_path);
    if (!file.is_open()) throw std::runtime_error("Cannot open genome file: " + file_path);
    std::string line;
    unsigned index = 0;
    while (std::getline(file, line)) {
        if (line[0] == '>') line = line.substr(1);
        std::string name = line.substr(0, line.find(' '));
        if (name.length() > 300) name = name.substr(0, 300);
        if (result.count(name)) {
            std::cerr << "Warning: Duplicate genome name: " << name << "\n";
            continue;
        }
        result[name] = index++;
        names.push_back(name);
    }
    return result;
}

struct PrimerInfo {
    int pos;
    int len;
    int plus;
    int minus;
};

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;

    size_t start = 0;
    while (true) {
        const size_t pos = s.find(delimiter, start);
        const size_t n = pos != std::string::npos ? pos - start : std::string::npos;
        tokens.push_back(s.substr(start, n));
        if (pos == std::string::npos)
            break;
        start = pos + 1;
    }
    return tokens;
}

int countMismatches(const std::string& mismatchField) {
    return std::count(mismatchField.begin(), mismatchField.end(), ':');
}

std::vector<int> getMutationPositions(const std::string& field) {
    std::vector<int> positions;
    std::regex pattern(R"((\d+):)");
    auto begin = std::sregex_iterator(field.begin(), field.end(), pattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        positions.push_back(std::stoi((*it)[1]));
    }
    return positions;
}

struct App {
public:
    void parseCliArgs(int argc, const char* argv[]);
    void readRefSequence();
    void loadTargetList();
    void loadBackgroundList();

    void alignPrimers();
    void alignPrimers(PrimerType primerType);

private:

    void runBowtie(const std::string& indexPath, const std::string& inputFastaPath, const std::string& outputPath);
    void processBowtieOutput(const std::string& bowtiePath);

private:
    Config m_cfg;

    std::string m_refSequence;
    std::vector<std::string> m_bowtieIndexPaths;
    std::vector<std::string> m_targetGenomeNames;
    std::unordered_map<std::string, unsigned> m_targetGenomeNameToIndex;
    std::unordered_map<std::string, unsigned> m_backgroundGenomeNameToIndex;

    // Transient state while processing one primer type
    PrimerType m_primerType = {};
    std::unique_ptr<std::ofstream> m_commonOut;
    std::unique_ptr<std::ofstream> m_specialOut;
};

void App::parseCliArgs(int argc, const char* argv[]) {
    parseArgs(argc, argv, m_cfg);

    std::stringstream ss(m_cfg.index);
    std::string token;
    while (std::getline(ss, token, ',')) m_bowtieIndexPaths.push_back(token);
}

void App::readRefSequence() {
    m_refSequence = readFastaSequence(m_cfg.ref_file);
}

void App::loadTargetList() {
    m_targetGenomeNameToIndex = m_cfg.common_file.empty()
        ? std::unordered_map<std::string, unsigned>()
        : loadGenomeIds(m_cfg.common_file, m_targetGenomeNames);
}

void App::loadBackgroundList() {
    std::vector<std::string> special_names;

    auto special_ids = m_cfg.special_file.empty()
        ? std::unordered_map<std::string, unsigned>()
        : loadGenomeIds(m_cfg.special_file, special_names);
}

void App::alignPrimers() {
    alignPrimers(PrimerType::inner);
    alignPrimers(PrimerType::outer);
    if (m_cfg.loop)
        alignPrimers(PrimerType::loop);
}

void App::alignPrimers(PrimerType primerType) {
    m_primerType = primerType;

    const std::string primerRegionsPath = m_cfg.dir + "/" + toString(m_primerType) + "/" + m_cfg.prefix;
    const std::string fastaPath = primerRegionsPath + ".fa";

    // Create input FASTA file
    {
        std::ifstream infile(primerRegionsPath);
        std::ofstream outfile(fastaPath);

        std::string line;
        while (std::getline(infile, line)) {
            std::smatch match;
            if (!std::regex_search(line, match, std::regex(R"(pos:(\d+)\tlength:(\d+)\t\+:(\d)\t-:(\d))"))) {
                std::cerr << "Could not parse line `" << line << "`" << std::endl;
                continue;
            }

            int pos = std::stoi(match[1]);
            int len = std::stoi(match[2]);
            std::string name = match[1].str() + "-" + match[2].str() + "-" + match[3].str() + "-" + match[4].str();
            std::string primer_seq = m_refSequence.substr(pos, len);
            outfile << ">" << name << "\n" << primer_seq << "\n";
        }
    }

    // Open output files
    if (!m_cfg.common_file.empty())
    {
        if (m_primerType == PrimerType::inner)
        {
            // Create common_list.txt
            std::ofstream commonListOut(primerRegionsPath + "-common_list.txt");
            for (size_t i = 0; i < m_targetGenomeNames.size(); i++)
                commonListOut << m_targetGenomeNames[i] + "\t" + std::to_string(i) + "\n";
        }

        m_commonOut = std::make_unique<std::ofstream>(primerRegionsPath + "-common.txt");
    }
    if ((!m_cfg.special_file.empty() || m_cfg.left))
    {
        m_specialOut = std::make_unique<std::ofstream>(primerRegionsPath + "-specific.txt");
    }

    for (const std::string& indexPath: m_bowtieIndexPaths)
    {
        const std::string bowtieOutputPath = primerRegionsPath + "_" + toString(m_primerType) + ".bowtie";
        runBowtie(indexPath, fastaPath, bowtieOutputPath);
        processBowtieOutput(bowtieOutputPath);
    }

    std::remove(fastaPath.c_str());
    m_commonOut.reset();
    m_specialOut.reset();
}

void App::runBowtie(const std::string& indexPath, const std::string& inputFastaPath, const std::string& outputPath) {
    const std::string v = std::to_string(m_cfg.mis_s);
    const std::string p = std::to_string(m_cfg.threads);
    std::vector<const char*> bowtieArgs {
        "bowtie", // executable name
        "-f",
        "--suppress", "5,6,7",
        "-v", v.c_str(),
        "-p", p.c_str(),
        "-a", indexPath.c_str(),
        inputFastaPath.c_str(),
        outputPath.c_str(),
    };

    bowtie(bowtieArgs.size(), bowtieArgs.data());
}

void App::processBowtieOutput(const std::string& bowtiePath) {
    std::ifstream file(bowtiePath);
    if (!file) throw std::runtime_error("Unable to open Bowtie output file: " + bowtiePath);
    std::string line;
    while (std::getline(file, line)) {
        auto fields = split(line, '\t');
        if (fields.size() < 5) continue;

        std::string primerName = fields[0];
        std::string strand = fields[1];
        std::string genomeId = fields[2].substr(0, std::min(300UL, fields[2].size()));
        std::string sequenceRead = fields[3];
        std::string mismatchField = fields[4];

        int mismatches = countMismatches(mismatchField);

        int pos, len, plus, minus;
        if (sscanf(primerName.c_str(), "%d-%d-%d-%d", &pos, &len, &plus, &minus) != 4) continue;

        bool begin = false, stop = false;
        auto mutationPositions = getMutationPositions(mismatchField);
        for (int mut : mutationPositions) {
            if (mut < 5) begin = true;
            if (mut >= len - 5) stop = true;
        }

        int strandMatchPlus = 0, strandMatchMinus = 0;

        if (m_primerType == PrimerType::inner) {
            if (plus && !begin) (strand == "+" ? strandMatchPlus : strandMatchMinus) = 1;
            if (minus && !stop)  (strand == "+" ? strandMatchMinus : strandMatchPlus) = 1;
        } else {
            if (plus && !stop)   (strand == "+" ? strandMatchPlus : strandMatchMinus) = 1;
            if (minus && !begin) (strand == "+" ? strandMatchMinus : strandMatchPlus) = 1;
        }

        if (strandMatchPlus + strandMatchMinus == 0) continue;

        const bool hasTargetList = !m_cfg.common_file.empty();
        if (hasTargetList && m_targetGenomeNameToIndex.count(genomeId)) {
            if (mismatches <= m_cfg.mis_c) {
                *m_commonOut << pos << '\t' << len << '\t' << m_targetGenomeNameToIndex.at(genomeId) << '\t' << sequenceRead
                        << '\t' << strandMatchPlus << '\t' << strandMatchMinus << '\n';

            }
            continue;
        }

        if (m_primerType == PrimerType::loop)
            continue;

        const bool hasExplicitBackgroundList = !m_cfg.special_file.empty();
        if (hasExplicitBackgroundList) {
            if (m_backgroundGenomeNameToIndex.count(genomeId)) {
                *m_specialOut << pos << '\t' << len << '\t' << m_backgroundGenomeNameToIndex.at(genomeId) << '\t' << sequenceRead
                        << '\t' << strandMatchPlus << '\t' << strandMatchMinus << '\n';
            }
            continue;
        }

        if (m_cfg.left) {
            if (m_backgroundGenomeNameToIndex.count(genomeId)) {
                *m_specialOut << pos << '\t' << len << '\t' << m_backgroundGenomeNameToIndex.at(genomeId) << '\t' << sequenceRead
                        << '\t' << strandMatchPlus << '\t' << strandMatchMinus << '\n';
            }
            else {
                const size_t backgroundGenomeIndex = m_backgroundGenomeNameToIndex.size();
                m_backgroundGenomeNameToIndex[genomeId] = backgroundGenomeIndex;
                *m_specialOut << pos << '\t' << len << '\t' << backgroundGenomeIndex << '\t' << sequenceRead
                        << '\t' << strandMatchPlus << '\t' << strandMatchMinus << '\n';
            }
        }
    }
}

int parpl_main(int argc, const char* argv[]) {
    App app;
    app.parseCliArgs(argc, argv);
    app.readRefSequence();
    app.loadTargetList();
    app.loadBackgroundList();
    app.alignPrimers();
    return 0;
}
