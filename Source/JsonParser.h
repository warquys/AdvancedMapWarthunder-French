#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <queue>
#include <condition_variable>

// Простой JSON парсер с поддержкой основных типов
namespace Json {
    enum class ValueType {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    class Value {
    public:
        ValueType type;
        
        // Данные в зависимости от типа
        bool boolValue;
        double numberValue;
        std::string stringValue;
        std::vector<std::shared_ptr<Value>> arrayValue;
        std::map<std::string, std::shared_ptr<Value>> objectValue;

        Value();
        Value(bool val);
        Value(double val);
        Value(const std::string& val);
        Value(const char* val);
        
        // Получение значений
        bool asBool() const;
        double asNumber() const;
        std::string asString() const;
        std::shared_ptr<Value> operator[](size_t index) const;
        std::shared_ptr<Value> operator[](const std::string& key) const;
        
        // Проверка типа
        bool isNull() const { return type == ValueType::Null; }
        bool isBool() const { return type == ValueType::Boolean; }
        bool isNumber() const { return type == ValueType::Number; }
        bool isString() const { return type == ValueType::String; }
        bool isArray() const { return type == ValueType::Array; }
        bool isObject() const { return type == ValueType::Object; }
        
        // Строковое представление
        std::string toString(int indent = 0) const;
    };

    // Парсинг JSON строки
    std::shared_ptr<Value> Parse(const std::string& json);
}

// Асинхронный JSON парсер
class JsonParser {
public:
    using ParseCallback = std::function<void(std::shared_ptr<Json::Value>, bool success, const std::string& error)>;
    
    JsonParser();
    ~JsonParser();
    
    // Добавить задачу на парсинг
    void ParseAsync(const std::string& jsonData, ParseCallback callback);
    
    // Добавить задачу на парсинг из файла
    void ParseFileAsync(const std::string& filePath, ParseCallback callback);
    
    // Остановить парсер
    void Stop();
    
    // Проверить, работает ли парсер
    bool IsRunning() const { return m_running; }
    
    // Получить количество задач в очереди
    size_t GetQueueSize() const;

private:
    struct ParseTask {
        std::string data;
        std::string filePath;
        bool isFile;
        ParseCallback callback;
    };
    
    void WorkerThread();
    std::string ReadFile(const std::string& filePath);
    
    std::thread m_workerThread;
    std::queue<ParseTask> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::atomic<bool> m_running;
    std::condition_variable m_condition;
};

