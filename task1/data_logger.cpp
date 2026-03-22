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

static const std::string API_URL      = "http://innov8dev.com/sampleapi/getdata.php";
static const std::string CSV_FILENAME = "data.csv";
static constexpr int FETCH_INTERVAL_SEC = 2;
static constexpr int TOTAL_DURATION_SEC = 600;
static constexpr int MST_OFFSET_HOURS  = -7;
static constexpr long CURL_TIMEOUT_SEC = 5L;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

bool httpGet(CURL* curl, const std::string& url, std::string& response)
{
    response.clear();
    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       CURL_TIMEOUT_SEC);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[HATA] HTTP: " << curl_easy_strerror(res) << "\n";
        return false;
    }
    return true;
}

std::string toMSTString(long long unixTimestamp)
{
    time_t mstTime = static_cast<time_t>(unixTimestamp + MST_OFFSET_HOURS * 3600LL);
    struct tm* tmInfo = std::gmtime(&mstTime);
    if (!tmInfo) return "0000-00-00 00:00:00";
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tmInfo);
    return std::string(buffer);
}

int main()
{
    std::cout << "=== Blebox Staj — Görev 1: Sensör Veri Kaydedici ===\n"
              << "API    : " << API_URL      << "\n"
              << "Süre   : " << TOTAL_DURATION_SEC << " sn"
              << " | Aralık: " << FETCH_INTERVAL_SEC << " sn\n"
              << "Çıktı  : " << CSV_FILENAME << "\n"
              << "─────────────────────────────────────────────────────\n";

    std::ofstream csvFile(CSV_FILENAME);
    if (!csvFile.is_open()) {
        std::cerr << "[KRİTİK] CSV dosyası açılamadı.\n";
        return EXIT_FAILURE;
    }
    csvFile << "date_time,temperature,rain\n";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[KRİTİK] CURL başlatılamadı.\n";
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    const TimePoint startTime = Clock::now();
    const auto totalDur  = std::chrono::seconds(TOTAL_DURATION_SEC);
    const auto interval  = std::chrono::seconds(FETCH_INTERVAL_SEC);

    int totalCount = 0, successCount = 0;

    while (Clock::now() - startTime < totalDur)
    {
        TimePoint nextTick = startTime + (totalCount * interval);
        std::this_thread::sleep_until(nextTick);
        if (Clock::now() - startTime >= totalDur) break;
        ++totalCount;

        std::string rawResponse;
        if (!httpGet(curl, API_URL, rawResponse) || rawResponse.empty()) {
            std::cerr << "[UYARI] İterasyon " << totalCount << ": veri alınamadı.\n";
            continue;
        }

        try {
            json data = json::parse(rawResponse);
            long long timestamp   = data.at("timestamp").get<long long>();
            double    temperature = data.at("temperature").get<double>();
            bool      isRaining   = data.at("rain").get<bool>();
            std::string mstTime   = toMSTString(timestamp);
            int rainFlag = isRaining ? 1 : 0;

            std::cout << "[" << std::setw(3) << totalCount << "] "
                      << mstTime
                      << " | Temp: " << std::fixed << std::setprecision(1) << temperature << "°C"
                      << " | Rain: " << rainFlag << "\n";

            csvFile << mstTime << ","
                    << std::fixed << std::setprecision(1) << temperature << ","
                    << rainFlag  << "\n";
            csvFile.flush();
            ++successCount;
        }
        catch (const json::parse_error& e) {
            std::cerr << "[HATA] JSON parse: " << e.what() << "\n";
        }
        catch (const json::type_error& e) {
            std::cerr << "[HATA] JSON tip: " << e.what() << "\n";
        }
        catch (const json::out_of_range& e) {
            std::cerr << "[HATA] JSON eksik alan: " << e.what() << "\n";
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    csvFile.close();

    std::cout << "\n=== Operasyon Tamamlandı ===\n"
              << "Toplam : " << totalCount   << "\n"
              << "Başarılı: " << successCount << "\n"
              << "CSV    : " << CSV_FILENAME << "\n";

    return EXIT_SUCCESS;
}