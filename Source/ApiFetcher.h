#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#endif

// Circuit Breaker для защиты от перегрузки API
struct CircuitBreaker {
    int consecutiveErrors = 0;
    std::chrono::steady_clock::time_point lastErrorTime;
    bool isOpen = false; // true = circuit открыт (заблокирован)
    static constexpr int MAX_ERRORS = 5; // Максимум ошибок перед блокировкой
    static constexpr int COOLDOWN_MS = 5000; // Время блокировки (5 секунд)
    
    bool CanMakeRequest() {
        if (!isOpen) return true;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastErrorTime);
        
        if (elapsed.count() >= COOLDOWN_MS) {
            // Пробуем снова после cooldown
            isOpen = false;
            consecutiveErrors = 0;
            return true;
        }
        return false;
    }
    
    void RecordSuccess() {
        consecutiveErrors = 0;
        isOpen = false;
    }
    
    void RecordError() {
        consecutiveErrors++;
        lastErrorTime = std::chrono::steady_clock::now();
        
        if (consecutiveErrors >= MAX_ERRORS) {
            isOpen = true;
        }
    }
};

// Асинхронный загрузчик данных из War Thunder API
class ApiFetcher {
public:
    using ChatCallback = std::function<void(const std::string& jsonData)>;
    using EventCallback = std::function<void(const std::string& jsonData)>;
    using IndicatorsCallback = std::function<void(const std::string& jsonData)>;
    using StateCallback = std::function<void(const std::string& jsonData)>;
    using MissionCallback = std::function<void(const std::string& jsonData)>;
    using MapInfoCallback = std::function<void(const std::string& jsonData)>;
    using MapObjectsCallback = std::function<void(const std::string& jsonData)>;
    
    ApiFetcher();
    ~ApiFetcher();
    
    // Установить callback'и для обработки данных
    void SetChatCallback(ChatCallback callback);
    void SetEventCallback(EventCallback callback);
    void SetIndicatorsCallback(IndicatorsCallback callback);
    void SetStateCallback(StateCallback callback);
    void SetMissionCallback(MissionCallback callback);
    void SetMapInfoCallback(MapInfoCallback callback);
    void SetMapObjectsCallback(MapObjectsCallback callback);
    
    // Начать/остановить загрузку
    void Start();
    void Stop();
    
    // Проверить, работает ли загрузчик
    bool IsRunning() const { return m_running; }
    
    // Получить/установить последние ID (для синхронизации)
    int GetLastChatId() const { 
        std::lock_guard<std::mutex> lock(m_idMutex);
        return m_lastChatId; 
    }
    int GetLastEventId() const { 
        std::lock_guard<std::mutex> lock(m_idMutex);
        return m_lastEventId; 
    }
    void SetLastChatId(int id) { 
        std::lock_guard<std::mutex> lock(m_idMutex);
        m_lastChatId = id; 
    }
    void SetLastEventId(int id) { 
        std::lock_guard<std::mutex> lock(m_idMutex);
        m_lastEventId = id; 
    }
    
private:
    void ChatWorkerThread();
    void EventWorkerThread();
    void IndicatorsWorkerThread();
    void StateWorkerThread();
    void MissionWorkerThread();
    void MapInfoWorkerThread();
    void MapObjectsWorkerThread();
    std::string HttpGet(const std::string& url, bool& success);
    std::string HttpGetWithRetry(const std::string& url, CircuitBreaker& breaker, int maxRetries = 2);
    
    // Connection pooling
    #ifdef _WIN32
    HINTERNET m_hInternet; // Переиспользуемое соединение
    std::mutex m_httpMutex; // Защита для thread-safe доступа к m_hInternet
    #endif
    
    // Circuit breakers для каждого endpoint
    CircuitBreaker m_chatBreaker;
    CircuitBreaker m_eventBreaker;
    CircuitBreaker m_indicatorsBreaker;
    CircuitBreaker m_stateBreaker;
    CircuitBreaker m_missionBreaker;
    CircuitBreaker m_mapInfoBreaker;
    CircuitBreaker m_mapObjectsBreaker;
    std::mutex m_breakerMutex; // Защита для circuit breakers
    
    std::thread m_chatWorkerThread;
    std::thread m_eventWorkerThread;
    std::thread m_indicatorsWorkerThread;
    std::thread m_stateWorkerThread;
    std::thread m_missionWorkerThread;
    std::thread m_mapInfoWorkerThread;
    std::thread m_mapObjectsWorkerThread;
    std::atomic<bool> m_running;
    ChatCallback m_chatCallback;
    EventCallback m_eventCallback;
    IndicatorsCallback m_indicatorsCallback;
    StateCallback m_stateCallback;
    MissionCallback m_missionCallback;
    MapInfoCallback m_mapInfoCallback;
    MapObjectsCallback m_mapObjectsCallback;
    std::mutex m_callbackMutex;
    
    mutable std::mutex m_idMutex;
    int m_lastChatId;
    int m_lastEventId;
};

