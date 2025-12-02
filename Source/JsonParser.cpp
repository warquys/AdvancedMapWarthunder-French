#include "JsonParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <condition_variable>
#ifdef _WIN32
#include <windows.h>
#endif

namespace Json {
    Value::Value() : type(ValueType::Null) {}
    
    Value::Value(bool val) : type(ValueType::Boolean), boolValue(val) {}
    
    Value::Value(double val) : type(ValueType::Number), numberValue(val) {}
    
    Value::Value(const std::string& val) : type(ValueType::String), stringValue(val) {}
    
    Value::Value(const char* val) : type(ValueType::String), stringValue(val) {}
    
    bool Value::asBool() const {
        if (type == ValueType::Boolean) return boolValue;
        if (type == ValueType::Number) return numberValue != 0.0;
        if (type == ValueType::String) return !stringValue.empty();
        return false;
    }
    
    double Value::asNumber() const {
        if (type == ValueType::Number) return numberValue;
        if (type == ValueType::Boolean) return boolValue ? 1.0 : 0.0;
        if (type == ValueType::String) {
            try {
                return std::stod(stringValue);
            } catch (...) {
                return 0.0;
            }
        }
        return 0.0;
    }
    
    std::string Value::asString() const {
        if (type == ValueType::String) return stringValue;
        if (type == ValueType::Number) return std::to_string(numberValue);
        if (type == ValueType::Boolean) return boolValue ? "true" : "false";
        if (type == ValueType::Null) return "null";
        return "";
    }
    
    std::shared_ptr<Value> Value::operator[](size_t index) const {
        if (type == ValueType::Array && index < arrayValue.size()) {
            return arrayValue[index];
        }
        return std::make_shared<Value>();
    }
    
    std::shared_ptr<Value> Value::operator[](const std::string& key) const {
        if (type == ValueType::Object) {
            auto it = objectValue.find(key);
            if (it != objectValue.end()) {
                return it->second;
            }
        }
        return std::make_shared<Value>();
    }
    
    std::string Value::toString(int indent) const {
        std::string indentStr(indent * 2, ' ');
        std::string nextIndentStr((indent + 1) * 2, ' ');
        
        switch (type) {
            case ValueType::Null:
                return "null";
            case ValueType::Boolean:
                return boolValue ? "true" : "false";
            case ValueType::Number:
                return std::to_string(numberValue);
            case ValueType::String:
                return "\"" + stringValue + "\"";
            case ValueType::Array: {
                std::string result = "[\n";
                for (size_t i = 0; i < arrayValue.size(); ++i) {
                    result += nextIndentStr + arrayValue[i]->toString(indent + 1);
                    if (i < arrayValue.size() - 1) result += ",";
                    result += "\n";
                }
                result += indentStr + "]";
                return result;
            }
            case ValueType::Object: {
                std::string result = "{\n";
                size_t i = 0;
                for (const auto& pair : objectValue) {
                    result += nextIndentStr + "\"" + pair.first + "\": " + pair.second->toString(indent + 1);
                    if (++i < objectValue.size()) result += ",";
                    result += "\n";
                }
                result += indentStr + "}";
                return result;
            }
        }
        return "";
    }
    
    // Простой JSON парсер
    class Parser {
    private:
        const char* m_data;
        size_t m_pos;
        size_t m_length;
        
        void SkipWhitespace() {
            while (m_pos < m_length && std::isspace(m_data[m_pos])) {
                m_pos++;
            }
        }
        
        char Current() const {
            return m_pos < m_length ? m_data[m_pos] : '\0';
        }
        
        char Next() {
            if (m_pos < m_length) m_pos++;
            return Current();
        }
        
        std::string ParseString() {
            if (Current() != '"') return "";
            Next(); // Skip opening quote
            
            std::string result;
            while (m_pos < m_length && Current() != '"') {
                if (Current() == '\\') {
                    Next();
                    switch (Current()) {
                        case 'n': result += '\n'; break;
                        case 't': result += '\t'; break;
                        case 'r': result += '\r'; break;
                        case '\\': result += '\\'; break;
                        case '"': result += '"'; break;
                        default: result += Current(); break;
                    }
                    Next();
                } else {
                    result += Current();
                    Next();
                }
            }
            if (Current() == '"') Next(); // Skip closing quote
            return result;
        }
        
        double ParseNumber() {
            std::string numStr;
            bool hasDot = false;
            
            if (Current() == '-') {
                numStr += Current();
                Next();
            }
            
            while (m_pos < m_length) {
                if (std::isdigit(Current())) {
                    numStr += Current();
                    Next();
                } else if (Current() == '.' && !hasDot) {
                    numStr += Current();
                    hasDot = true;
                    Next();
                } else {
                    break;
                }
            }
            
            try {
                return std::stod(numStr);
            } catch (...) {
                return 0.0;
            }
        }
        
        std::shared_ptr<Value> ParseValue() {
            SkipWhitespace();
            
            switch (Current()) {
                case 'n': // null
                    if (m_length - m_pos >= 4 && 
                        m_data[m_pos] == 'n' && m_data[m_pos+1] == 'u' && 
                        m_data[m_pos+2] == 'l' && m_data[m_pos+3] == 'l') {
                        m_pos += 4;
                        return std::make_shared<Value>();
                    }
                    break;
                    
                case 't': // true
                    if (m_length - m_pos >= 4 && 
                        m_data[m_pos] == 't' && m_data[m_pos+1] == 'r' && 
                        m_data[m_pos+2] == 'u' && m_data[m_pos+3] == 'e') {
                        m_pos += 4;
                        return std::make_shared<Value>(true);
                    }
                    break;
                    
                case 'f': // false
                    if (m_length - m_pos >= 5 && 
                        m_data[m_pos] == 'f' && m_data[m_pos+1] == 'a' && 
                        m_data[m_pos+2] == 'l' && m_data[m_pos+3] == 's' && 
                        m_data[m_pos+4] == 'e') {
                        m_pos += 5;
                        return std::make_shared<Value>(false);
                    }
                    break;
                    
                case '"': {
                    auto val = std::make_shared<Value>(ParseString());
                    return val;
                }
                    
                case '[': {
                    auto arr = std::make_shared<Value>();
                    arr->type = ValueType::Array;
                    Next(); // Skip '['
                    SkipWhitespace();
                    
                    if (Current() != ']') {
                        while (true) {
                            arr->arrayValue.push_back(ParseValue());
                            SkipWhitespace();
                            if (Current() == ']') break;
                            if (Current() != ',') break;
                            Next(); // Skip ','
                        }
                    }
                    if (Current() == ']') Next(); // Skip ']'
                    return arr;
                }
                
                case '{': {
                    auto obj = std::make_shared<Value>();
                    obj->type = ValueType::Object;
                    Next(); // Skip '{'
                    SkipWhitespace();
                    
                    if (Current() != '}') {
                        while (true) {
                            SkipWhitespace();
                            if (Current() != '"') break;
                            std::string key = ParseString();
                            SkipWhitespace();
                            if (Current() != ':') break;
                            Next(); // Skip ':'
                            obj->objectValue[key] = ParseValue();
                            SkipWhitespace();
                            if (Current() == '}') break;
                            if (Current() != ',') break;
                            Next(); // Skip ','
                        }
                    }
                    if (Current() == '}') Next(); // Skip '}'
                    return obj;
                }
                
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': {
                    return std::make_shared<Value>(ParseNumber());
                }
            }
            
            return std::make_shared<Value>(); // null
        }
        
    public:
        Parser(const std::string& json) 
            : m_data(json.c_str()), m_pos(0), m_length(json.length()) {}
        
        std::shared_ptr<Value> Parse() {
            SkipWhitespace();
            return ParseValue();
        }
    };
    
    std::shared_ptr<Value> Parse(const std::string& json) {
        Parser parser(json);
        return parser.Parse();
    }
}

// Реализация JsonParser
JsonParser::JsonParser() : m_running(true) {
    m_workerThread = std::thread(&JsonParser::WorkerThread, this);
}

JsonParser::~JsonParser() {
    Stop();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void JsonParser::ParseAsync(const std::string& jsonData, ParseCallback callback) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    ParseTask task;
    task.data = jsonData;
    task.isFile = false;
    task.callback = callback;
    m_taskQueue.push(task);
    m_condition.notify_one();
}

void JsonParser::ParseFileAsync(const std::string& filePath, ParseCallback callback) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    ParseTask task;
    task.filePath = filePath;
    task.isFile = true;
    task.callback = callback;
    m_taskQueue.push(task);
    m_condition.notify_one();
}

void JsonParser::Stop() {
    m_running = false;
    m_condition.notify_all();
}

size_t JsonParser::GetQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

void JsonParser::WorkerThread() {
    // Привязываем поток к отдельному ядру (если доступно)
    // В Windows можно использовать SetThreadAffinityMask
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 0x1); // Привязка к первому ядру
    #endif
    
    while (m_running) {
        ParseTask task;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { 
                return !m_taskQueue.empty() || !m_running; 
            });
            
            if (!m_running && m_taskQueue.empty()) break;
            
            if (!m_taskQueue.empty()) {
                task = m_taskQueue.front();
                m_taskQueue.pop();
            }
        }
        
        if (task.callback) {
            try {
                std::string jsonData = task.isFile ? ReadFile(task.filePath) : task.data;
                
                if (task.isFile && jsonData.empty()) {
                    task.callback(nullptr, false, "Failed to read file: " + task.filePath);
                    continue;
                }
                
                auto result = Json::Parse(jsonData);
                task.callback(result, true, "");
            } catch (const std::exception& e) {
                task.callback(nullptr, false, std::string("Parse error: ") + e.what());
            } catch (...) {
                task.callback(nullptr, false, "Unknown parse error");
            }
        }
    }
}

std::string JsonParser::ReadFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

