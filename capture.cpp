// Datei: capture.cpp
#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Wir öffnen jedes /dev/video*-Device und fragen mit VIDIOC_QUERYCAP 
// die Basis‑Capabilities ab (Treiber, Kartenname, Bus-Info, Version, 
// Capability‑Flags).
// 
// Mit VIDIOC_ENUM_FMT listen wir alle Pixelformate auf und geben 
// für jedes die Beschreibung und den FourCC‑Code aus.
// 
// Für jedes Format rufen wir VIDIOC_ENUM_FRAMESIZES auf, 
// um alle unterstützten Auflösungen zu ermitteln, und für 
// jede diskrete Größe rufen wir VIDIOC_ENUM_FRAMEINTERVALS auf, 
// um mögliche Frameraten zu listen.


// probt alle cameras mit index 0 bis 10 zu öffnen
std::vector<int> findCameras() {
    std::vector<int> cams = {};

    // Maximal so viele Indizes testen (0…9)
    const int maxTested = 10;

    bool foundAny = false;
    for (int idx = 0; idx < maxTested; ++idx) {
        // Versuch, Kamera idx zu öffnen (Backend V4L2)
        cv::VideoCapture cap(idx, cv::CAP_V4L2);
        if (cap.isOpened()) {
            std::cout << "✔ Kamera gefunden bei Index " << idx << std::endl;
            cap.release();
            foundAny = true;
            cams.push_back(idx);
        }
    }

    if (!foundAny) 
        std::cerr << "✘ Keine Kameras gefunden (Indices 0–" << (maxTested-1) << ").\n";

    return cams;
}

// Hilfsfunktion: liest und gibt alle Framegrößen und 
// -intervalle für einen gegebenen Pixelformat aus
void enumerateFrameSizes(int fd, __u32 pixfmt) {
    v4l2_frmsizeenum fsize;
    fsize.index = 0;
    fsize.pixel_format = pixfmt;

    while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize) == 0) {
        if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            std::cout << "      Größe: "
                      << fsize.discrete.width << "x" << fsize.discrete.height << "\n";
            // nun Intervalle für diese Größe
            v4l2_frmivalenum fival;
            fival.index = 0;
            fival.pixel_format = pixfmt;
            fival.width = fsize.discrete.width;
            fival.height = fsize.discrete.height;
            while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival) == 0) {
                if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                    std::cout << "        Intervall: "
                              << fival.discrete.numerator << "/"
                              << fival.discrete.denominator << " s\n";
                }
                ++fival.index;
            }
        }
        else if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
            std::cout << "      Schrittweite-Größenbereich: min "
                      << fsize.stepwise.min_width << "x" << fsize.stepwise.min_height
                      << " – max " << fsize.stepwise.max_width << "x" << fsize.stepwise.max_height
                      << " in Schritten "
                      << fsize.stepwise.step_width << "x" << fsize.stepwise.step_height
                      << "\n";
        }
        ++fsize.index;
    }
}

// Listet alle Kameras und deren Eigenschaften auf
void listCameraInfo() {
    const std::string devDir = "/dev";
    DIR* dp = opendir(devDir.c_str());
    if (!dp) {
        std::perror("opendir");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dp))) {
        std::string name = entry->d_name;
        if (name.rfind("video", 0) == 0) {
            std::string devPath = devDir + "/" + name;
            int fd = open(devPath.c_str(), O_RDWR | O_NONBLOCK, 0);
            if (fd < 0) {
                std::cerr << "  X Konnte " << devPath << " nicht öffnen\n";
                continue;
            }

            std::cout << "=== Gerät: " << devPath << " ===\n";

            // Capabilities auslesen
            v4l2_capability cap;
            if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
                std::cout << "  Treiber:       " << cap.driver << "\n";
                std::cout << "  Karten-Name:   " << cap.card << "\n";
                std::cout << "  Bus-Info:      " << cap.bus_info << "\n";
                std::cout << "  Version:       "
                          << ((cap.version >> 16) & 0xFF) << "."
                          << ((cap.version >> 8) & 0xFF) << "."
                          << (cap.version & 0xFF) << "\n";
                std::cout << "  Capabilities:  0x" << std::hex << cap.capabilities << std::dec << "\n";
            } else {
                std::cerr << "  X VIDIOC_QUERYCAP fehlgeschlagen\n";
                close(fd);
                continue;
            }

            // unterstützte Formate
            std::cout << "  Unterstützte Formate:\n";
            v4l2_fmtdesc fmt;
            fmt.index = 0;
            fmt.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
                std::cout << "    Format " << fmt.index
                          << ": " << fmt.description
                          << " (0x" << std::hex << fmt.pixelformat << std::dec << ")\n";
                // Framegrößen und -intervalle für dieses Format
                enumerateFrameSizes(fd, fmt.pixelformat);
                ++fmt.index;
            }

            std::cout << std::endl;
            close(fd);
        }
    }
    closedir(dp);
}

int main(int argc, char* argv[]) {
    std::vector<int> cams = findCameras();

    listCameraInfo();

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