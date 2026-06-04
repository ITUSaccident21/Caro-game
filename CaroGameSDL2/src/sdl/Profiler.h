#pragma once
#include <chrono>
#include <string>
#include <map>
#include <iostream>

// ================================================================
//  Profiler.h — Simple profiling tool for performance measurement
//  Đo lường thời gian thực thi hàm và detect bottlenecks
// ================================================================

class Profiler {
public:
    struct ProfileData {
        std::chrono::milliseconds totalTime{0};
        int callCount = 0;
        std::chrono::milliseconds minTime{999999};
        std::chrono::milliseconds maxTime{0};

        double avgTime() const {
            return callCount > 0 ? totalTime.count() / (double)callCount : 0.0;
        }
    };

private:
    static std::map<std::string, ProfileData>& GetData() {
        static std::map<std::string, ProfileData> data;
        return data;
    }

    struct ScopedTimer {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;

        ScopedTimer(const std::string& n) : name(n) {
            start = std::chrono::high_resolution_clock::now();
        }

        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            auto& data = GetData();
            data[name].totalTime += duration;
            data[name].callCount++;
            data[name].minTime = std::min(data[name].minTime, duration);
            data[name].maxTime = std::max(data[name].maxTime, duration);
        }
    };

public:
    // Bắt đầu profile một block code
    static ScopedTimer Profile(const std::string& name) {
        return ScopedTimer(name);
    }

    // In báo cáo profiling
    static void PrintReport() {
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                    PROFILING REPORT                            ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << std::left 
                  << std::setw(25) << "Function"
                  << std::setw(12) << "Calls"
                  << std::setw(12) << "Total (ms)"
                  << std::setw(12) << "Avg (ms)"
                  << std::setw(12) << "Min (ms)"
                  << std::setw(12) << "Max (ms)" << "\n";
        std::cout << "─────────────────────────────────────────────────────────────────\n";

        auto& data = GetData();
        for (auto& [name, profile] : data) {
            std::cout << std::left
                      << std::setw(25) << name
                      << std::setw(12) << profile.callCount
                      << std::setw(12) << profile.totalTime.count()
                      << std::setw(12) << std::fixed << std::setprecision(2) << profile.avgTime()
                      << std::setw(12) << profile.minTime.count()
                      << std::setw(12) << profile.maxTime.count() << "\n";
        }
        std::cout << "─────────────────────────────────────────────────────────────────\n";
    }

    static void Reset() {
        GetData().clear();
    }

    static void PrintBottlenecks() {
        std::cout << "\n📊 TOP BOTTLENECKS:\n";
        auto& data = GetData();

        std::vector<std::pair<std::string, double>> sorted;
        for (auto& [name, profile] : data) {
            sorted.push_back({name, profile.totalTime.count()});
        }

        std::sort(sorted.rbegin(), sorted.rend());

        for (size_t i = 0; i < std::min(size_t(5), sorted.size()); i++) {
            std::cout << (i+1) << ". " << sorted[i].first 
                      << " → " << sorted[i].second << "ms\n";
        }
    }
};

// Macro để dễ sử dụng
#define PROFILE_SCOPE(name) auto _profile = Profiler::Profile(name)

// Sử dụng:
// {
//     PROFILE_SCOPE("MyFunction");
//     // code cần đo
// }
