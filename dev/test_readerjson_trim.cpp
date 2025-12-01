// dev/test_readerjson_trim.cpp
#include <iostream>
#include <string>

#include "io/ReaderJSON.hpp"

int main() {
    try {
        std::cout << "=== ReaderJSON basic file + trim test ===\n";

        // Test 1: Read file
        std::cout << "\n1. Reading JSON file...\n";
        io::ReaderJSON reader("data/config.json");
        std::string content = reader.read_file();
        std::cout << "File contents:\n" << content << "\n";

        // Test 2: Trim function
        std::cout << "\n2. Testing trim()...\n";
        std::cout << "   '  hello  ' -> '" << reader.trim("  hello  ") << "'\n";
        std::cout << "   'world'     -> '" << reader.trim("world") << "'\n";
        std::cout << "   '   '       -> '" << reader.trim("   ") << "'\n";

        std::cout << "\nAll basic ReaderJSON tests completed.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
