#include <iomanip>
#include <string>
#include <cstdint>
#include <string>
#include <algorithm>
#include <iostream>

struct Entry {
    int day;
    int month;
    int year;

    std::string name;
    std::string surname;
    std::string patron;

    int id;

    int orig_row;

    friend auto operator<=>(const Entry& l, const Entry& r) {
        return std::tie(l.surname, l.name, l.patron, l.id) <=> std::tie(r.surname, r.name, r.patron, r.id);
    }
};

struct Key {
    std::string name;
    std::string surname;
    std::string patron;

    int id;

    friend auto operator<=>(const Key& l, const Key& r) {
        return std::tie(l.surname, l.name, l.patron, l.id) <=> std::tie(r.surname, r.name, r.patron, r.id);
    }

    friend bool operator==(const Key& l, const Key& r) {
        return std::tie(l.surname, l.name, l.patron, l.id) == std::tie(r.surname, r.name, r.patron, r.id);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Key& k) {
    return os << k.surname << ' ' << k.name << ' ' << k.patron << " #" << k.id;
}

inline std::ostream& operator<<(std::ostream& os, const Entry& e) {
    os << e.surname << ' ' << e.name << ' ' << e.patron << " #" << e.id << ", "
       << std::setfill('0')
       << std::setw(2) << e.day << ' ' << std::setw(2) << e.month << ' ' << e.year
       << std::setfill(' ')
       << ", row " << e.orig_row;
    return os;
}

namespace detail {
inline uint64_t fold(const std::string& s, size_t block = 4) {
    uint64_t sum = 0;
    for (size_t i = 0; i < s.size(); i += block) {
        uint64_t part = 0;
        size_t end = std::min(i + block, s.size());
        for (size_t j = i; j < end; ++j)
            part = (part << 8) | static_cast<unsigned char>(s[j]);
        sum += part;
    }
    return sum;
}
} // namespace detail

template <>
struct std::hash<Key> {
    size_t operator()(const Key& k) const noexcept {
        std::string s = k.surname + '|' + k.name + '|' + k.patron + '|' + std::to_string(k.id);
        return static_cast<size_t>(detail::fold(s));
    }
};
