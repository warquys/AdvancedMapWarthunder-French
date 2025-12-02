#include "ApiFetcher.h"
#include <windows.h>
#include <wininet.h>
#include <sstream>
#include <thread>

#pragma comment(lib, "wininet.lib")

ApiFetcher::ApiFetcher() 
    : m_running(false)
    , m_lastChatId(0)
    , m_lastEventId(0)
{
    #ifdef _WIN32
    // Инициализируем connection pool (переиспользуемое соединение)
    m_hInternet = InternetOpenA("WarThunderAdvanced/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (m_hInternet) {
        // Устанавливаем таймауты для connection pool
        DWORD timeout = 5000; // 5 секунд
        InternetSetOptionA(m_hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionA(m_hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionA(m_hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    }
    #endif
}

ApiFetcher::~ApiFetcher() {
    Stop();
    if (m_chatWorkerThread.joinable()) {
        m_chatWorkerThread.join();
    }
    if (m_eventWorkerThread.joinable()) {
        m_eventWorkerThread.join();
    }
    if (m_indicatorsWorkerThread.joinable()) {
        m_indicatorsWorkerThread.join();
    }
    if (m_stateWorkerThread.joinable()) {
        m_stateWorkerThread.join();
    }
    if (m_missionWorkerThread.joinable()) {
        m_missionWorkerThread.join();
    }
    if (m_mapInfoWorkerThread.joinable()) {
        m_mapInfoWorkerThread.join();
    }
    if (m_mapObjectsWorkerThread.joinable()) {
        m_mapObjectsWorkerThread.join();
    }
    
    #ifdef _WIN32
    // Закрываем connection pool
    if (m_hInternet) {
        InternetCloseHandle(m_hInternet);
        m_hInternet = nullptr;
    }
    #endif
}

void ApiFetcher::SetChatCallback(ChatCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_chatCallback = callback;
}

void ApiFetcher::SetEventCallback(EventCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_eventCallback = callback;
}

void ApiFetcher::SetIndicatorsCallback(IndicatorsCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_indicatorsCallback = callback;
}

void ApiFetcher::SetStateCallback(StateCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_stateCallback = callback;
}

void ApiFetcher::SetMissionCallback(MissionCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_missionCallback = callback;
}

void ApiFetcher::SetMapInfoCallback(MapInfoCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_mapInfoCallback = callback;
}

void ApiFetcher::SetMapObjectsCallback(MapObjectsCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_mapObjectsCallback = callback;
}

void ApiFetcher::Start() {
    if (m_running) return;
    
    m_running = true;
    
    // Запускаем отдельные потоки для чата, событий, indicators, state, mission и map_info
    m_chatWorkerThread = std::thread(&ApiFetcher::ChatWorkerThread, this);
    m_eventWorkerThread = std::thread(&ApiFetcher::EventWorkerThread, this);
    m_indicatorsWorkerThread = std::thread(&ApiFetcher::IndicatorsWorkerThread, this);
    m_stateWorkerThread = std::thread(&ApiFetcher::StateWorkerThread, this);
    m_missionWorkerThread = std::thread(&ApiFetcher::MissionWorkerThread, this);
    m_mapInfoWorkerThread = std::thread(&ApiFetcher::MapInfoWorkerThread, this);
    m_mapObjectsWorkerThread = std::thread(&ApiFetcher::MapObjectsWorkerThread, this);
    
    // Привязываем потоки к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(m_chatWorkerThread.native_handle(), 0x1);
    SetThreadAffinityMask(m_eventWorkerThread.native_handle(), 0x1);
    SetThreadAffinityMask(m_indicatorsWorkerThread.native_handle(), 0x1);
    SetThreadAffinityMask(m_stateWorkerThread.native_handle(), 0x1);
    SetThreadAffinityMask(m_missionWorkerThread.native_handle(), 0x1);
    SetThreadAffinityMask(m_mapInfoWorkerThread.native_handle(), 0x1);
    SetThreadAffinityMask(m_mapObjectsWorkerThread.native_handle(), 0x1);
    #endif
}

void ApiFetcher::Stop() {
    m_running = false;
}

// HttpGet с connection pooling и улучшенной обработкой ошибок
std::string ApiFetcher::HttpGet(const std::string& url, bool& success) {
    success = false;
    
    #ifdef _WIN32
    std::lock_guard<std::mutex> lock(m_httpMutex);
    
    // Используем переиспользуемое соединение (connection pooling)
    if (!m_hInternet) {
        // Если соединение потеряно, пытаемся пересоздать
        m_hInternet = InternetOpenA("WarThunderAdvanced/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!m_hInternet) {
            return "";
        }
        DWORD timeout = 5000;
        InternetSetOptionA(m_hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionA(m_hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionA(m_hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    }
    
    HINTERNET hConnect = InternetOpenUrlA(m_hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        // Проверяем тип ошибки
        DWORD error = GetLastError();
        if (error == ERROR_INTERNET_TIMEOUT || error == ERROR_INTERNET_CONNECTION_ABORTED) {
            // Временная ошибка - можно повторить
        } else if (error == ERROR_INTERNET_NAME_NOT_RESOLVED || error == ERROR_INTERNET_CANNOT_CONNECT) {
            // Игра не запущена или API недоступен
        }
        return "";
    }
    
    std::string result;
    char buffer[4096];
    DWORD bytesRead;
    
    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }
    
    InternetCloseHandle(hConnect);
    
    // Проверяем, что получили данные
    if (!result.empty() && result.find("error") == std::string::npos) {
        success = true;
    }
    
    return result;
    #else
    return "";
    #endif
}

// HttpGet с retry логикой и circuit breaker
std::string ApiFetcher::HttpGetWithRetry(const std::string& url, CircuitBreaker& breaker, int maxRetries) {
    // Проверяем circuit breaker
    {
        std::lock_guard<std::mutex> lock(m_breakerMutex);
        if (!breaker.CanMakeRequest()) {
            return ""; // Circuit открыт - пропускаем запрос
        }
    }
    
    // Пытаемся выполнить запрос с retry
    for (int attempt = 0; attempt <= maxRetries; attempt++) {
        bool success = false;
        std::string result = HttpGet(url, success);
        
        {
            std::lock_guard<std::mutex> lock(m_breakerMutex);
            if (success) {
                breaker.RecordSuccess();
                return result;
            } else {
                breaker.RecordError();
            }
        }
        
        // Если не последняя попытка, ждем перед retry (exponential backoff)
        if (attempt < maxRetries) {
            int delayMs = 100 * (1 << attempt); // 100ms, 200ms, 400ms...
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }
    
    return "";
}

void ApiFetcher::ChatWorkerThread() {
    // Привязываем поток к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
    #endif
    
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(2000); // Обновление каждые 2 секунды (Communication: 2-5 seconds)
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed >= updateInterval) {
            lastUpdate = now;
            
            // Получаем игровой чат
            int lastChatId;
            {
                std::lock_guard<std::mutex> lock(m_idMutex);
                lastChatId = m_lastChatId;
            }
            
            std::string chatUrl = "http://localhost:8111/gamechat?lastId=" + std::to_string(lastChatId);
            std::string chatData = HttpGetWithRetry(chatUrl, m_chatBreaker);
            
            // Если получили данные - обрабатываем
            if (!chatData.empty()) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_chatCallback) {
                    m_chatCallback(chatData);
                }
            }
        }
        
        // Небольшая задержка, чтобы не нагружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ApiFetcher::EventWorkerThread() {
    // Привязываем поток к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
    #endif
    
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(2000); // Обновление каждые 2 секунды (Communication: 2-5 seconds)
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed >= updateInterval) {
            lastUpdate = now;
            
            // Получаем события
            int lastEventId;
            {
                std::lock_guard<std::mutex> lock(m_idMutex);
                lastEventId = m_lastEventId;
            }
            
            std::string eventUrl = "http://localhost:8111/hudmsg?lastEvt=0&lastDmg=" + std::to_string(lastEventId);
            std::string eventData = HttpGetWithRetry(eventUrl, m_eventBreaker);
            
            // Если получили данные - обрабатываем
            if (!eventData.empty()) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_eventCallback) {
                    m_eventCallback(eventData);
                }
            }
        }
        
        // Небольшая задержка, чтобы не нагружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ApiFetcher::IndicatorsWorkerThread() {
    // Привязываем поток к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
    #endif
    
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(150); // Critical flight data: 100-200ms (рекомендуется 100-200ms)
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed >= updateInterval) {
            lastUpdate = now;
            
            std::string indicatorsUrl = "http://localhost:8111/indicators";
            std::string indicatorsData = HttpGetWithRetry(indicatorsUrl, m_indicatorsBreaker);
            
            // Если получили данные - обрабатываем
            if (!indicatorsData.empty()) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_indicatorsCallback) {
                    m_indicatorsCallback(indicatorsData);
                }
            }
        }
        
        // Небольшая задержка, чтобы не нагружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void ApiFetcher::StateWorkerThread() {
    // Привязываем поток к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
    #endif
    
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(150); // Critical flight data: 100-200ms (рекомендуется 100-200ms)
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed >= updateInterval) {
            lastUpdate = now;
            
            std::string stateUrl = "http://localhost:8111/state";
            std::string stateData = HttpGetWithRetry(stateUrl, m_stateBreaker);
            
            // Если получили данные - обрабатываем
            if (!stateData.empty()) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_stateCallback) {
                    m_stateCallback(stateData);
                }
            }
        }
        
        // Небольшая задержка, чтобы не нагружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void ApiFetcher::MissionWorkerThread() {
    // Привязываем поток к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
    #endif
    
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(1500); // Strategic information: 1-2 seconds (рекомендуется 1-2 секунды)
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed >= updateInterval) {
            lastUpdate = now;
            
            std::string missionUrl = "http://localhost:8111/mission.json";
            std::string missionData = HttpGetWithRetry(missionUrl, m_missionBreaker);
            
            // Если получили данные - обрабатываем
            if (!missionData.empty()) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_missionCallback) {
                    m_missionCallback(missionData);
                }
            }
        }
        
        // Небольшая задержка, чтобы не нагружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ApiFetcher::MapInfoWorkerThread() {
    // Привязываем поток к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
    #endif
    
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(2000); // Обновление каждые 2 секунды (map_info редко меняется)
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed >= updateInterval) {
            lastUpdate = now;
            
            std::string mapInfoUrl = "http://localhost:8111/map_info.json";
            std::string mapInfoData = HttpGetWithRetry(mapInfoUrl, m_mapInfoBreaker);
            
            // Если получили данные - обрабатываем
            if (!mapInfoData.empty()) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_mapInfoCallback) {
                    m_mapInfoCallback(mapInfoData);
                }
            }
        }
        
        // Небольшая задержка, чтобы не нагружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ApiFetcher::MapObjectsWorkerThread() {
    // Привязываем поток к первому ядру
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
    #endif
    
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(750); // Tactical information: 500-1000ms (рекомендуется 500-1000ms для map_obj.json)
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed >= updateInterval) {
            lastUpdate = now;
            
            std::string mapObjectsUrl = "http://localhost:8111/map_obj.json";
            std::string mapObjectsData = HttpGetWithRetry(mapObjectsUrl, m_mapObjectsBreaker);
            
            // Если получили данные - обрабатываем
            if (!mapObjectsData.empty()) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_mapObjectsCallback) {
                    m_mapObjectsCallback(mapObjectsData);
                }
            }
        }
        
        // Небольшая задержка, чтобы не нагружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

