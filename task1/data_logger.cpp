/**
 * @file    data_logger.cpp
 * @author  Serhat Varli — Kocaeli University, Electronics and Communication Eng.
 * @brief   Blebox Erasmus Internship — Task 1
 *
 *          Fetches temperature and rain data from innov8dev.com API
 *          every 2 seconds for 10 minutes and saves results to a CSV file.
 *
 * @build   g++ data_logger.cpp -o data_logger -lcurl -std=c++17
 *
 * Dependencies:
 *   - libcurl4-openssl-dev   (HTTP requests)
 *   - nlohmann-json3-dev     (JSON parsing — header-only, no -l flag needed)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ─── Constants ──────────────────────────────────────────────────────────────
static const std::string API_URL      = "http://innov8dev.com/sampleapi/getdata.php";
static const std::string CSV_FILENAME = "data.csv";
static constexpr int  FETCH_INTERVAL_SEC = 2;    // Interval between measurements (s)
static constexpr int  TOTAL_DURATION_SEC = 600;  // Total collection time: 10 min
static constexpr int  MST_OFFSET_HOURS  = -7;   // MST = UTC-7
static constexpr long CURL_TIMEOUT_SEC  = 5L;   // HTTP request timeout (s)

// ─── libcurl Write Callback ──────────────────────────────────────────────────
/**
 * @brief Called by libcurl each time a chunk of HTTP response data arrives.
 *        Appends the chunk to the user-provided std::string buffer.
 */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// ─── HTTP GET ────────────────────────────────────────────────────────────────
/**
 * @brief Sends an HTTP GET request to the given URL.
 *        The curl handle is reused across calls to avoid repeated init/cleanup overhead.
 *
 * @param curl      Pre-initialised CURL handle
 * @param url       Target URL
 * @param response  On success, the response body is written here
 * @return          true on success, false on error
 */
bool httpGet(CURL* curl, const std::string& url, std::string& response)
{
    response.clear();
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        CURL_TIMEOUT_SEC);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow HTTP redirects

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[ERROR] HTTP: " << curl_easy_strerror(res) << "\n";
        return false;
    }
    return true;
}

// ─── Timestamp Conversion ────────────────────────────────────────────────────
/**
 * @brief Converts a Unix timestamp to MST (UTC-7) and returns a formatted string.
 *        Uses gmtime() to remain independent of the OS local timezone.
 *
 * @param unixTimestamp  Epoch seconds from API "timestamp" field
 * @return               Formatted string: "yyyy-MM-dd HH:mm:ss"
 */
std::string toMSTString(long long unixTimestamp)
{
    // Apply MST offset manually: UTC → MST
    time_t mstTime = static_cast<time_t>(unixTimestamp + MST_OFFSET_HOURS * 3600LL);
    struct tm* tmInfo = std::gmtime(&mstTime);
    if (!tmInfo) return "0000-00-00 00:00:00";
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tmInfo);
    return std::string(buffer);
}

// ─── Main ────────────────────────────────────────────────────────────────────
int main()
{
    std::cout << "=== Blebox Internship — Task 1: Sensor Data Logger ===\n"
              << "API    : " << API_URL      << "\n"
              << "Duration: " << TOTAL_DURATION_SEC << " s"
              << " | Interval: " << FETCH_INTERVAL_SEC << " s\n"
              << "Output : " << CSV_FILENAME << "\n"
              << "─────────────────────────────────────────────────────\n";

    // Open CSV file and write header
    std::ofstream csvFile(CSV_FILENAME);
    if (!csvFile.is_open()) {
        std::cerr << "[CRITICAL] Cannot open CSV file: " << CSV_FILENAME << "\n";
        return EXIT_FAILURE;
    }
    csvFile << "date_time,temperature,rain\n";

    // Initialise libcurl (must be called once per program)
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[CRITICAL] Failed to initialise CURL.\n";
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    // Timing: steady_clock is monotonic — unaffected by system clock changes
    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    const TimePoint startTime = Clock::now();
    const auto totalDur = std::chrono::seconds(TOTAL_DURATION_SEC);
    const auto interval = std::chrono::seconds(FETCH_INTERVAL_SEC);

    int totalCount = 0, successCount = 0;

    while (Clock::now() - startTime < totalDur)
    {
        // sleep_until keeps the interval fixed regardless of how long the HTTP
        // request takes — unlike sleep_for which would cause cumulative drift
        TimePoint nextTick = startTime + (totalCount * interval);
        std::this_thread::sleep_until(nextTick);
        if (Clock::now() - startTime >= totalDur) break;
        ++totalCount;

        std::string rawResponse;
        if (!httpGet(curl, API_URL, rawResponse) || rawResponse.empty()) {
            std::cerr << "[WARNING] Iteration " << totalCount << ": no data, skipping.\n";
            continue;
        }

        try {
            json data = json::parse(rawResponse);

            long long  timestamp   = data.at("timestamp").get<long long>();
            double     temperature = data.at("temperature").get<double>();
            bool       isRaining   = data.at("rain").get<bool>();
            std::string mstTime    = toMSTString(timestamp);
            int        rainFlag    = isRaining ? 1 : 0;

            std::cout << "[" << std::setw(3) << totalCount << "] "
                      << mstTime
                      << " | Temp: " << std::fixed << std::setprecision(1)
                      << temperature << "°C"
                      << " | Rain: " << rainFlag << "\n";

            csvFile << mstTime << ","
                    << std::fixed << std::setprecision(1) << temperature << ","
                    << rainFlag  << "\n";

            // flush() writes immediately to disk — protects data on power loss
            csvFile.flush();
            ++successCount;
        }
        catch (const json::parse_error& e) {
            std::cerr << "[ERROR] JSON parse: " << e.what() << "\n";
        }
        catch (const json::type_error& e) {
            std::cerr << "[ERROR] JSON type mismatch: " << e.what() << "\n";
        }
        catch (const json::out_of_range& e) {
            std::cerr << "[ERROR] JSON missing field: " << e.what() << "\n";
        }
    }

    // Release all resources
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    csvFile.close();

    std::cout << "\n=== Operation Complete ===\n"
              << "Total iterations : " << totalCount   << "\n"
              << "Successful       : " << successCount << "\n"
              << "CSV saved        : " << CSV_FILENAME << "\n";

    return EXIT_SUCCESS;
}