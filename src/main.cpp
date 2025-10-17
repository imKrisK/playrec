#include "capture_engine.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    std::cout << "PlayRec - Game Capture Application\n";
    std::cout << "==================================\n\n";

    // Create capture engine
    playrec::CaptureEngine engine;

    // Configure capture settings
    playrec::CaptureSettings settings;
    settings.target_fps = 60;
    settings.quality = playrec::Quality::HIGH;
    settings.capture_audio = true;
    settings.capture_cursor = true;
    settings.output_path = "gameplay_capture.mp4";

    // Parse command line arguments (basic implementation)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--fps" && i + 1 < argc) {
            settings.target_fps = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            settings.output_path = argv[++i];
        } else if (arg == "--codec" && i + 1 < argc) {
            settings.codec = argv[++i];
        } else if (arg == "--no-audio") {
            settings.capture_audio = false;
        } else if (arg == "--no-cursor") {
            settings.capture_cursor = false;
        } else if (arg == "--quality" && i + 1 < argc) {
            std::string quality_str = argv[++i];
            if (quality_str == "low") settings.quality = playrec::Quality::LOW;
            else if (quality_str == "medium") settings.quality = playrec::Quality::MEDIUM;
            else if (quality_str == "high") settings.quality = playrec::Quality::HIGH;
            else if (quality_str == "ultra") settings.quality = playrec::Quality::ULTRA;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --fps <number>      Target FPS (default: 60)\n";
            std::cout << "  --output <file>     Output file path (default: gameplay_capture.mp4)\n";
            std::cout << "  --codec <codec>     Video codec: h264|h265 (default: h264)\n";
            std::cout << "  --quality <level>   Quality: low|medium|high|ultra (default: high)\n";
            std::cout << "  --no-audio          Disable audio capture\n";
            std::cout << "  --no-cursor         Disable cursor capture\n";
            std::cout << "  --help, -h          Show this help message\n";
            return 0;
        }
    }

    // Display settings
    std::cout << "Capture Settings:\n";
    std::cout << "  FPS: " << settings.target_fps << "\n";
    std::cout << "  Audio: " << (settings.capture_audio ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Cursor: " << (settings.capture_cursor ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Output: " << settings.output_path << "\n\n";

    // Initialize capture engine
    if (!engine.initialize(settings)) {
        std::cerr << "Error: Failed to initialize capture engine\n";
        return 1;
    }

    std::cout << "Capture engine initialized successfully!\n";
    std::cout << "Press Enter to start capturing, then Enter again to stop...\n";
    std::cin.get();

    // Start capture
    if (!engine.start_capture()) {
        std::cerr << "Error: Failed to start capture\n";
        return 1;
    }

    std::cout << "Capture started! Recording to: " << settings.output_path << "\n";
    std::cout << "Press Enter to stop...\n";

    // Monitor capture while running
    auto start_time = std::chrono::high_resolution_clock::now();
    while (engine.is_capturing()) {
        // Check for user input (non-blocking would be better)
        if (std::cin.peek() != EOF && std::cin.peek() == '\n') {
            std::cin.get();
            break;
        }

        // Display stats every second
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
        
        if (elapsed.count() > 0 && elapsed.count() % 1 == 0) {
            auto stats = engine.get_stats();
            std::cout << "\rFrames: " << stats.frames_captured 
                      << " | FPS: " << std::fixed << std::setprecision(1) << stats.average_fps
                      << " | Dropped: " << stats.frames_dropped 
                      << " | Size: " << (stats.file_size_bytes / 1024 / 1024) << " MB" << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop capture
    std::cout << "\nStopping capture...\n";
    engine.stop_capture();

    // Final stats
    auto final_stats = engine.get_stats();
    std::cout << "\nCapture completed!\n";
    std::cout << "Final Statistics:\n";
    std::cout << "  Total frames captured: " << final_stats.frames_captured << "\n";
    std::cout << "  Frames dropped: " << final_stats.frames_dropped << "\n";
    std::cout << "  Average FPS: " << std::fixed << std::setprecision(2) << final_stats.average_fps << "\n";
    std::cout << "  File size: " << (final_stats.file_size_bytes / 1024.0 / 1024.0) << " MB\n";
    std::cout << "  Output saved to: " << settings.output_path << "\n";

    return 0;
}