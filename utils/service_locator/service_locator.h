#pragma once
#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

namespace utils
{
/// Thread-safe registry of singleton services by type.
/// registerService<T>(...) — create and register a singleton service of type T.
/// get<T>() — retrieve a shared_ptr<T>.
/// has<T>() — check whether a service of type T is registered.
class ServiceLocator final
{
   public:
    template <typename T, typename... Args>
    void registerService(Args&&... args)
    {
        std::lock_guard lock(mtx_);
        auto key = std::type_index(typeid(T));
        if (services_.find(key) != services_.end())
        {
            throw std::runtime_error("Service already registered");
        }
        services_.emplace(key, std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T>
    std::shared_ptr<T> get()
    {
        std::lock_guard lock(mtx_);
        auto key = std::type_index(typeid(T));
        auto it = services_.find(key);
        if (it == services_.end())
        {
            throw std::runtime_error("Service not found");
        }
        return std::static_pointer_cast<T>(it->second);
    }

    template <typename T>
    bool has() noexcept
    {
        std::lock_guard lock(mtx_);
        return services_.find(std::type_index(typeid(T))) != services_.end();
    }

   private:
    // Note: Using std::type_index as a key in unordered_map is generally fine,
    // but may have performance implications with a large number of types.
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
    std::mutex mtx_;
};
};  // namespace utils
