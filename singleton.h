#pragma once

#include <memory>

namespace uia::testing
{
  template <typename T>
  class Singleton
  {
  public:
    static T &instance()
    {
      static const std::unique_ptr<T> instance{new T{token{}}};
      return *instance;
    }

    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton) = delete;
    virtual ~Singleton() = default;

  protected:
    struct token
    {
    }; // helper class
    Singleton(){};
    // Singleton() noexcept = default;
  };
} // namespace rpad