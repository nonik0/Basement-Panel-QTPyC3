#pragma once

#include <Arduino.h>
#include <stdexcept>
#include <vector>

template <typename T>
class Stack {
private:
    std::vector<T> data;

public:
    void push(const T& value) {
        data.push_back(value);
    }

    void pop() {
        if (data.empty()) {
            throw std::out_of_range("Stack is empty");
        }
        data.pop_back();
    }

    T& top() {
        if (data.empty()) {
            throw std::out_of_range("Stack is empty");
        }
        return data.back();
    }

    bool isEmpty() const {
        return data.empty();
    }

    size_t size() const {
        return data.size();
    }
};

template <typename T>
class Queue {
private:
    std::vector<T> data;

public:
    void enqueue(const T& value) {
        data.push_back(value);
    }

    void dequeue() {
        if (data.empty()) {
            throw std::out_of_range("Queue is empty");
        }
        data.erase(data.begin());
    }

    T& front() {
        if (data.empty()) {
            throw std::out_of_range("Queue is empty");
        }
        return data.front();
    }

    bool isEmpty() const {
        return data.empty();
    }

    size_t size() const {
        return data.size();
    }
};

template <typename T>
class Set {
private:
    std::vector<T> data;

public:
    void add(const T& value) {
        if (!contains(value)) {
            data.push_back(value);
        }
    }

    void remove(const T& value) {
        data.erase(std::remove(data.begin(), data.end(), value), data.end());
    }

    bool contains(const T& value) const {
        return std::find(data.begin(), data.end(), value) != data.end();
    }

    size_t size() const {
        return data.size();
    }
};