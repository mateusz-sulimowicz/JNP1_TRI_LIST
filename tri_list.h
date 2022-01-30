#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <initializer_list>
#include <variant>
#include <vector>
#include <ranges>
#include <tuple>
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
    return [=](T x) mutable {
        return f(g(x));
    };
};

template<typename T1, typename T2, typename T3>
class tri_list {
    using var_t = std::variant<T1, T2, T3>;

    template<typename T>
    using mod = std::function<T(T)>;

    class iterator {
        using iter_t = typename std::vector<var_t>::const_iterator;
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::iter_difference_t<iter_t>;
        using value_type = var_t;
        using reference = var_t;

        iterator() = default;

        explicit iterator(const std::tuple<mod<T1>, mod<T2>, mod<T3>> &m,
                          iter_t original) noexcept
                : modifiers(m), it(original) {}

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

        var_t operator*() const noexcept {
            return get_modified(*it);
        }

    private:
        std::tuple<mod<T1>, mod<T2>, mod<T3>> modifiers = {identity<T1>, identity<T2>, identity<T3>};
        iter_t it = std::vector<var_t>().cbegin();

        var_t get_modified(var_t v) const {
            if (std::holds_alternative<T1>(v)) {
                return var_t{std::get<mod<T1>>(modifiers)(std::get<T1>(v))};
            } else if (std::holds_alternative<T2>(v)) {
                return var_t{std::get<mod<T2>>(modifiers)(std::get<T2>(v))};
            } else {
                return var_t{std::get<mod<T3>>(modifiers)(std::get<T3>(v))};
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
        std::get<mod<T>>(modifiers) = compose<T>(m, std::get<mod<T>>(modifiers));
    }

    template<one_of<T1, T2, T3> T>
    void reset() {
        std::get<mod<T>>(modifiers) = identity<T>;
    }

    template<one_of<T1, T2, T3> T>
    auto range_over() const {
        return content
               | std::views::filter([](var_t v) { return std::holds_alternative<T>(v); })
               | std::views::transform([this](var_t v) { return std::get<mod<T>>(modifiers)(std::get<T>(v)); });
    }

    auto begin() const {
        return iterator(modifiers, content.cbegin());
    }

    auto end() const {
        return iterator(modifiers, content.cend());
    }

private:
    std::vector<var_t> content;
    std::tuple<mod<T1>, mod<T2>, mod<T3>> modifiers = {identity<T1>, identity<T2>, identity<T3>};
};

#endif //TRI_LIST_H
