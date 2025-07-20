// Datei: capture.cpp
#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    // Gerät 0 öffnen (meist die eingebaute Webcam)
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Fehler: Konnte Kamera nicht öffnen\n";
        return 1;
    }

    // Ein Frame einlesen
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        std::cerr << "Fehler: Kein Bild erhalten\n";
        return 1;
    }

    // Bild unter dem gegebenen Dateinamen speichern oder default
    std::string filename = "snapshot.jpg";
    if (argc >= 2) {
        filename = argv[1];
    }
    if (!cv::imwrite(filename, frame)) {
        std::cerr << "Fehler: Konnte Bild nicht speichern\n";
        return 1;
    }

    std::cout << "Bild gespeichert als „" << filename << "“\n";
    return 0;
}