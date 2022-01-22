#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <initializer_list>
#include <variant>
#include <vector>
#include "tri_list_concepts.h"
#include <iostream>

template<typename T>
static auto identity = [](T x) { return x; };

template<typename T>
auto compose(modifier<T> auto f, modifier<T> auto g) {
    return [=](T x) { return f(g(x)); };
};

template<typename T1, typename T2, typename T3>
class tri_list {
    using var_t = std::variant<T1, T2, T3>;
    using t1_modifier_t = std::function<T1(T1)>;
    using t2_modifier_t = std::function<T2(T2)>;
    using t3_modifier_t = std::function<T3(T3)>;
public:
    tri_list() = default;

    tri_list(std::initializer_list<var_t> list) : content(list) {}

    template<typename T>
    void push_back(const T &t) {
        content.push_back(var_t{t});
    }

    template<typename T, modifier<T> F>
    void modify_only(F m = F{}) {
        if constexpr (std::is_same_v<T, T1>) {
            m1 = compose<T>(m, m1);
        } else if constexpr(std::is_same_v<T, T2>)
            m2 = compose<T>(m, m2);
        else {
            m3 = compose<T>(m, m3);
        }
    }

    template<typename T>
    void reset() {
        if constexpr(std::is_same_v<T, T1>) {
            m1 = identity<T1>;
        } else if constexpr(std::is_same_v<T, T2>) {
            m2 = identity<T2>;
        } else if constexpr(std::is_same_v<T, T3>) {
            m3 = identity<T3>;
        }
    }

    template<typename T>
    auto range_over() {
        return content
               | std::views::filter([](var_t v) { return std::holds_alternative<T>(v); })
               | std::views::transform([this](var_t v) { return get_modifier<T>()(std::get<T>(v)); });
    }

    auto begin() {
        return (content | std::views::transform([this](var_t v) {
            return var_t{m1(std::get<T1>(v))};
        })).begin();
    }

    auto end() {
        return (content | std::views::transform([this](var_t v) {
            return var_t{m1(std::get<T1>(v))};
        })).end();
    }

private:
    std::vector<var_t> content;
    t1_modifier_t m1 = identity<T1>;
    t2_modifier_t m2 = identity<T2>;
    t3_modifier_t m3 = identity<T3>;

    template<typename T>
    auto get_modifier() const {
        if constexpr (std::is_same_v<T, T1>) {
            return m1;
        } else if constexpr (std::is_same_v<T, T2>) {
            return m2;
        } else if constexpr (std::is_same_v<T, T2>) {
            return m3;
        } else {
            return identity<T>;
        }
    }
};

#endif //TRI_LIST_H
