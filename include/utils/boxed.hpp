#pragma once

#include <memory>

template<typename T>
class Boxed : private std::unique_ptr<T> {
public:
  Boxed() = default;

  Boxed(const T& value) : std::unique_ptr<T>(new T(value)) {}
  Boxed(T&& value) : std::unique_ptr<T>(new T(std::move(value))) {}

  explicit Boxed(T* ptr) : std::unique_ptr<T>(ptr) {}
  explicit Boxed(std::unique_ptr<T>&& ptr) : std::unique_ptr<T>(std::move(ptr)) {}

  Boxed(const Boxed& other) : std::unique_ptr<T>(std::make_unique<T>(*other)) {}

  Boxed(Boxed&& other) = default;
  Boxed& operator=(Boxed&& other) = default;

  using std::unique_ptr<T>::get;
  using std::unique_ptr<T>::operator*;
  using std::unique_ptr<T>::operator->;

  friend bool operator==(const Boxed& left, const Boxed& right) {
    return *left == *right;
  }
};