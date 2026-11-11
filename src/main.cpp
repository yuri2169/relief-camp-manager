#include "ConsoleApp.h"
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    // The data directory can be overridden, which keeps the tests and the
    // demo from fighting over data/.
    std::string dataDir = (argc > 1) ? argv[1] : "data";
    try {
        ConsoleApp app(dataDir);
        app.load();
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
