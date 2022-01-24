#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <initializer_list>
#include <variant>
#include <vector>
#include <ranges>
#include <functional>
#include "tri_list_concepts.h"

template<typename T, typename T1, typename T2, typename T3>
concept one_of = requires(T t) {
    requires (std::is_same_v<T, T1> && !std::is_same_v<T, T2> && !std::is_same_v<T, T3>)
    || (!std::is_same_v<T, T1> && std::is_same_v<T, T2> && !std::is_same_v<T, T3>)
    || (!std::is_same_v<T, T1> && !std::is_same_v<T, T2> && std::is_same_v<T, T3>);
};

template<typename T>
constinit static const auto identity = [](T x) { return x; };

template<typename T, modifier<T> F, modifier<T> G>
std::function<T(T)> compose(F f, G g) {
    return [=](T x) {
        return f(g(x));
    };
};

template<typename T1, typename T2, typename T3>
class tri_list {
    using var_t = std::variant<T1, T2, T3>;
    using t1_modifier_t = std::function<T1(T1)>;
    using t2_modifier_t = std::function<T2(T2)>;
    using t3_modifier_t = std::function<T3(T3)>;

    class iterator : public std::vector<var_t>::iterator {
        using iter_t = typename std::vector<var_t>::iterator;
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::iter_difference_t<iter_t>;
        using value_type = var_t;
        using pointer = var_t;
        using reference = var_t;

        iterator() = default;

        explicit iterator(const t1_modifier_t &m1,
                          const t2_modifier_t &m2,
                          const t3_modifier_t &m3,
                          iter_t original) noexcept
                : m1(m1), m2(m2), m3(m3), it(original) {}

        iterator &operator++() noexcept {
            ++it;
            return *this;
        }

        iterator operator++(int) noexcept {
            iterator retval = *this;
            ++(*this);
            return retval;
        }

        iterator &operator--() noexcept {
            --it;
            return *this;
        }

        iterator operator--(int) noexcept {
            iterator retval = *this;
            --(*this);
            return retval;
        }

        bool operator==(const iterator &other) const noexcept {
            return it == other.it;
        }

        bool operator!=(const iterator &other) const noexcept {
            return !(*this == other);
        }

        reference operator*() const noexcept {
            return modify(*it);
        }

        pointer operator->() const noexcept {
            return modify(*it);
        }

    private:
        t1_modifier_t m1 = identity<T1>;
        t2_modifier_t m2 = identity<T2>;
        t3_modifier_t m3 = identity<T3>;
        iter_t it = std::vector<var_t>().begin();

        var_t modify(var_t v) const {
            if (std::holds_alternative<T1>(v)) {
                return var_t{m1(std::get<T1>(v))};
            } else if (std::holds_alternative<T2>(v)) {
                return var_t{m2(std::get<T2>(v))};
            } else {
                return var_t{m3(std::get<T3>(v))};
            }
        }
    };

public:
    tri_list() = default;

    tri_list(std::initializer_list<var_t> list) : content(list) {}

    template<one_of<T1, T2, T3> T>
    void push_back(const T &t) {
        content.push_back(var_t{t});
    }

    template<one_of<T1, T2, T3> T, modifier<T> F>
    void modify_only(F m = F{}) {
        if constexpr (std::is_same_v<T, T1>) {
            m1 = compose<T>(m, m1);
        } else if constexpr (std::is_same_v<T, T2>) {
            m2 = compose<T>(m, m2);
        } else if constexpr (std::is_same_v<T, T3>) {
            m3 = compose<T>(m, m3);
        }
    }

    template<one_of<T1, T2, T3> T>
    void reset() {
        if constexpr(std::is_same_v<T, T1>) {
            m1 = identity<T1>;
        } else if constexpr(std::is_same_v<T, T2>) {
            m2 = identity<T2>;
        } else if constexpr(std::is_same_v<T, T3>) {
            m3 = identity<T3>;
        }
    }

    template<one_of<T1, T2, T3> T>
    auto range_over() {
        return content
               | std::views::filter([](var_t v) { return std::holds_alternative<T>(v); })
               | std::views::transform([this](var_t v) { return get_modifier<T>()(std::get<T>(v)); });
    }

    auto begin() {
        return iterator(m1, m2, m3, content.begin());
    }

    auto end() {
        return iterator(m1, m2, m3, content.end());
    }

private:
    std::vector<var_t> content;
    t1_modifier_t m1 = identity<T1>;
    t2_modifier_t m2 = identity<T2>;
    t3_modifier_t m3 = identity<T3>;

    template<typename T>
    requires one_of<T, T1, T2, T3>
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
