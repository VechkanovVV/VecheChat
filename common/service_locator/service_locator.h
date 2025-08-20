#pragma once
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <stdexcept>

/// Сервис-локатор: позволяет регистрировать и получать объекты по типу.
/// Использует std::type_index как ключ и std::shared_ptr для хранения объектов.
/// Проверяет повторную регистрацию и отсутствие сервиса при запросе.
class ServiceLocator final {
public:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    template <typename T, typename... Args>  
    void registerService(Args&&... args){
        auto key = std::type_index(typeid(T));
        if (services_.count(key) != 0) {
            throw std::runtime_error("The object has already been created!");
        }
        services_.emplace(key, std::make_shared<T>(std::forward<Args>(args)...));
    }
    
    template <typename T>
    std::shared_ptr<T> get() {
        auto key = std::type_index(typeid(T));
        auto it = services_.find(key);
        if (it == services_.end()) {
            throw std::runtime_error("The object does not exist!");
        }
        
        return std::static_pointer_cast<T>(it->second);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
};