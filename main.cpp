#include <iostream>
#include <thread>
#include "CaptureManager.h"

int main() {
    CaptureManager cm;
    cm.startCapture();

    // Wait until capture finishes
    std::this_thread::sleep_for(std::chrono::seconds(35));

    cm.stopCapture();
    std::cout << "Recording complete\n";
    return 0;
}
