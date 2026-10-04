#include <chrono>
#include <iostream>
#include <string>
#include <thread>

// A child-process fixture checks the same protocol used by the real .NET helper.
int main(int argc, char** argv)
{
    const std::string mode = argc > 2 ? argv[2] : "valid";
    std::string command;
    while (std::getline(std::cin, command) && command == "sample") {
        if (mode == "hang") { std::this_thread::sleep_for(std::chrono::seconds(10)); continue; }
        if (mode == "exit") return 1;
        if (mode == "invalid") std::cout << "bad json\n";
        else std::cout << R"({"version":1,"cpu":[{"name":"CPU","sensor":"CPU Package","celsius":52}],"gpu":[]})" << '\n';
        std::cout.flush();
    }
}
