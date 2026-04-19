#pragma once

#include <string>

#include "config/types/app.hpp"

namespace edgevision::config {

    struct CommandLineParseResult {
        AppConfig config{};
        std::string error{};

        [[nodiscard]] bool parsed() const;
    };

    [[nodiscard]] CommandLineParseResult parseCommandLine(
        int argc,
        char* argv[],
        const AppConfig& defaults = AppConfig{}
    );

} // namespace edgevision::config
