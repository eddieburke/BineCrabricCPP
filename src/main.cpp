#include "net/minecraft/client/Minecraft.hpp"
#include "seedfinder/runtime/RuntimeInit.hpp"

#include <exception>
#include <iostream>
#include <string_view>

#ifdef _WIN32
#include "net/minecraft/client/lifecycle/ClientInitializer.hpp"
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
    net::minecraft::client::lifecycle::installCrashDiagnostics();
#endif

    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i] != nullptr ? std::string_view(argv[i]) : std::string_view();
        if (arg == "--headless" || arg == "--no-gui") {
            headless = true;
        }
    }

    if (headless) {
        std::cout << "Headless validation is being moved out of the client executable.\n";
        return 0;
    }

    try {
#ifdef _WIN32
        net::minecraft::client::lifecycle::setStartupPhase("main: seedfinder runtime");
#endif
        seedfinder::runtime::initialize();
#ifdef _WIN32
        net::minecraft::client::lifecycle::setStartupPhase("main: starting client");
#endif
        const int exitCode = net::minecraft::client::Minecraft::main(argc, argv);
        seedfinder::runtime::shutdown();
        return exitCode;
    } catch (const std::exception& exception) {
        const std::string details = std::string("Uncaught exception in main: ") + exception.what();
#ifdef _WIN32
        net::minecraft::client::lifecycle::reportFatalError("Minecraft Native - startup failed", details);
        net::minecraft::client::lifecycle::pauseBeforeExit();
#endif
        std::cerr << details << std::endl;
        return 1;
    } catch (...) {
        const std::string details = "Uncaught unknown exception in main.";
#ifdef _WIN32
        net::minecraft::client::lifecycle::reportFatalError("Minecraft Native - startup failed", details);
        net::minecraft::client::lifecycle::pauseBeforeExit();
#endif
        std::cerr << details << std::endl;
        return 1;
    }
}
