#include <iostream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <cstdlib>
#include <string>
#include <stdexcept>

enum class Action {
    START,
    STOP,
    FORCE_STOP,
};

std::optional<Action> parseAction(const std::string &action) {
    if (action == "start") { return Action::START; }
    if (action == "stop") { return Action::STOP; }
    if (action == "force-stop") { return Action::FORCE_STOP; }
    return std::nullopt;
}

std::optional<int> findLxcId(const std::string &hostname) {
    const std::filesystem::path lxcPath = "/etc/pve/lxc";

    if (!std::filesystem::is_directory(lxcPath)) {
        std::cerr << "LXC directory doesn't exist\n";
        return std::nullopt;
    }

    for (const auto &entry : std::filesystem::directory_iterator(lxcPath)) {

        if (entry.path().extension() != ".conf") {
            continue;
        }

        std::ifstream file(entry.path());

        if (!file.is_open()) {
            continue;
        }

        std::string line;

        while (std::getline(file, line)) {

            const std::string prefix = "hostname: ";

            if (line.starts_with(prefix)) {
                std::string currentHostname =
                    line.substr(prefix.length());

                if (currentHostname == hostname) {
                    try{
                        int id = std::stoi(
                        entry.path().stem().string());
                        return id;
                    }
                    catch (const std::invalid_argument&) {
                        continue;
                    }
                    catch (const std::out_of_range&) {
                        continue;
                    }
                }
            }
        }
    }
    return std::nullopt;
}

int doAction(const std::string &argument, const std::string &name) {
    const auto id = findLxcId(name);

    if (!id) {
        std::cout << "Unknown LXC: " << name << std::endl;
        return 1;
    }

    const std::string cmd = argument + " " + std::to_string(*id);
    const int result = std::system(cmd.c_str());

    if (result != 0) {
        std::cerr << "Command failed" << std::endl;;
        return 1;
    }

    std::cout << " " << name  << "(" << std::to_string(*id) << ")..." << std::endl;

    return 0;
}

int main(const int argc, char *argv[]) {
    if (argc != 4) {
        std::cout << "Usage: homemox <lxc/vm> <action> <name>" << std::endl;
        return 1;
    }

    const std::string typeArg = argv[1];
    const std::string actionArg = argv[2];
    const std::string nameArg = argv[3];

    auto action = parseAction(actionArg);

    if (!action) {
        std::cout << "Unknown action: " << actionArg << std::endl;
        return 1;
    }
    if (typeArg != "lxc" && typeArg != "vm") {
        std::cout << "Unknown type: " << typeArg << std::endl;
        return 1;
    }

    switch (*action) {
        case Action::START:
            if (typeArg == "lxc") {
                std::cout << "Starting";
                return doAction("pct start", nameArg);
            }
            return 1;
        case Action::STOP:
            if (typeArg == "lxc") {
                std::cout << "Stopping";
                return doAction("pct shutdown", nameArg);
            }
            return 1;
        case Action::FORCE_STOP:
            if (typeArg == "lxc") {
                std::cout << "Stopping (with force)";
                return doAction("pct stop", nameArg);
            }
            return 1;
    }
    return 1;
}
