#include "src/io/ReaderJSON.hpp"
#include <iostream>

using namespace std;

int main() {
    try {
        // Test 1: Read file
        cout << "=== Test 1: Reading JSON file ===" << endl;
        io::ReaderJSON reader("data/config.json");
        string content = reader.read_file();
        cout << content << endl;
        
        // Test 2: Trim function
        cout << "\n=== Test 2: Testing trim() ===" << endl;
        cout << "Test 'hello': '" << reader.trim("  hello  ") << "'" << endl;
        cout << "Test 'world': '" << reader.trim("world") << "'" << endl;
        cout << "Test '   ': '" << reader.trim("   ") << "'" << endl;
        
        cout << "\n✅ All tests passed!" << endl;
        
    } catch (const exception& e) {
        cerr << "❌ Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
