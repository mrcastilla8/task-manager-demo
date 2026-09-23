#pragma once

#include <exception>
#include <iostream>
#include <string>

namespace test {

inline int& assertionCount() {
    static int count = 0;
    return count;
}

inline int& failureCount() {
    static int count = 0;
    return count;
}

inline void expect(bool condition, const char* expression, const char* file,
                   int line) {
    ++assertionCount();
    if (!condition) {
        ++failureCount();
        std::cerr << file << ':' << line << ": fallo: " << expression << '\n';
    }
}

template <typename Exception, typename Function>
void expectThrows(Function&& function, const char* expression, const char* file,
                  int line) {
    ++assertionCount();
    try {
        function();
    } catch (const Exception&) {
        return;
    } catch (const std::exception& exception) {
        ++failureCount();
        std::cerr << file << ':' << line << ": " << expression
                  << " lanzo una excepcion inesperada: " << exception.what()
                  << '\n';
        return;
    } catch (...) {
        ++failureCount();
        std::cerr << file << ':' << line << ": " << expression
                  << " lanzo una excepcion desconocida\n";
        return;
    }

    ++failureCount();
    std::cerr << file << ':' << line << ": " << expression
              << " no lanzo la excepcion esperada\n";
}

inline int finish() {
    if (failureCount() == 0) {
        std::cout << assertionCount() << " comprobaciones correctas\n";
        return 0;
    }

    std::cerr << failureCount() << " de " << assertionCount()
              << " comprobaciones fallaron\n";
    return 1;
}

}  // namespace test

#define EXPECT_TRUE(expression) \
    ::test::expect(static_cast<bool>(expression), #expression, __FILE__, __LINE__)
#define EXPECT_FALSE(expression) EXPECT_TRUE(!(expression))
#define EXPECT_EQ(actual, expected) \
    ::test::expect((actual) == (expected), #actual " == " #expected, __FILE__, \
                   __LINE__)
#define EXPECT_THROW(expression, exception_type)                       \
    ::test::expectThrows<exception_type>([&]() { (void)(expression); }, \
                                         #expression, __FILE__, __LINE__)
