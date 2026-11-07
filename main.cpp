#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "hashmap.hpp"

namespace {
using Map = hashmap<Key, Entry>;

std::size_t table_size(std::size_t n) {
    while (std::gcd(n, std::size_t{2}) != 1) ++n;
    return n;
}

Key key_of(const Entry& e) { return {e.name, e.surname, e.patron, e.id}; }

bool same_entry(const Entry& a, const Entry& b) {
    return a.day == b.day && a.month == b.month && a.year == b.year &&
           a.name == b.name && a.surname == b.surname && a.patron == b.patron &&
           a.id == b.id && a.orig_row == b.orig_row;
}

bool parse_line(const std::string& line, Entry& e) {
    std::istringstream in(line);
    return static_cast<bool>(in >> e.day >> e.month >> e.year
                                >> e.surname >> e.name >> e.patron >> e.id);
}

int read_int(const char* prompt, int min_value, int max_value) {
    for (;;) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) throw std::runtime_error("input ended");
        try {
            std::size_t used = 0;
            int v = std::stoi(line, &used);
            if (used == line.size() && v >= min_value && v <= max_value) return v;
        } catch (...) {}
        std::cout << "Enter an integer from " << min_value << " to " << max_value << ".\n";
    }
}

void run_hashmap_self_tests() {
    using TestMap = hashmap<int, std::string>;

    TestMap table(11);
    if (!table.insert(1, "one")) throw std::runtime_error("self-test failed: insert");
    if (!table.insert(12, "twelve")) throw std::runtime_error("self-test failed: collision insert");
    if (table.insert(1, "duplicate")) throw std::runtime_error("self-test failed: duplicate key accepted");

    const std::string* collision_value = table.get(12);
    if (!collision_value || *collision_value != "twelve" || table.last_probe_count() != 2)
        throw std::runtime_error("self-test failed: collision lookup or probe count");
    if (table.get(23) != nullptr || table.last_probe_count() != 3)
        throw std::runtime_error("self-test failed: missing-key lookup or probe count");

    if (!table.remove(1)) throw std::runtime_error("self-test failed: remove existing key");
    collision_value = table.get(12);
    if (!collision_value || *collision_value != "twelve")
        throw std::runtime_error("self-test failed: lookup past deleted collision");
    if (table.remove(99)) throw std::runtime_error("self-test failed: removed missing key");

    if (!table.insert(23, "twenty-three")) throw std::runtime_error("self-test failed: insert after deletion");
    if (!table.get(12) || !table.get(23))
        throw std::runtime_error("self-test failed: collision keys after tombstone reuse");
}
} // namespace

int main(int argc, char** argv) {
    try {
        run_hashmap_self_tests();
        std::cout << "Hashmap self-tests passed.\n";

        int n;
        std::size_t initial_size;
        Key sought;
        if (argc == 7) {
            n = std::stoi(argv[1]);
            initial_size = static_cast<std::size_t>(std::stoull(argv[2]));
            sought = {argv[3], argv[4], argv[5], std::stoi(argv[6])};
        } else {
            n = read_int("Number of input rows (10..1000000): ", 10, 1000000);
            int cap = read_int("Initial table size (positive integer): ", 1, 1000000000);
            initial_size = static_cast<std::size_t>(cap);
            std::cout << "Search key (surname name patronymic application_number): ";
            if (!(std::cin >> sought.surname >> sought.name >> sought.patron >> sought.id))
                throw std::runtime_error("invalid search key");
        }
        if (n < 10 || n > 1000000 || initial_size == 0)
            throw std::runtime_error("n must be 10..1000000 and initial size must be positive");

        std::ifstream input("input.txt");
        if (!input) throw std::runtime_error("cannot open input.txt");
        Map table(initial_size);
        std::vector<Entry> loaded;
        loaded.reserve(static_cast<std::size_t>(n));
        int duplicates = 0;
        std::string line;
        for (int row = 1; row <= n; ++row) {
            if (!std::getline(input, line)) throw std::runtime_error("input.txt has fewer than n rows");
            Entry e{};
            if (!parse_line(line, e))
                throw std::runtime_error("expected: DD MM YYYY surname name patronymic application_number");
            e.orig_row = row;
            if (!table.insert(key_of(e), e)) ++duplicates;
            else loaded.push_back(e);
        }

        Entry* found = table.get(sought);
        std::cout << "Search result: ";
        if (found) std::cout << "input row " << found->orig_row << '\n';
        else std::cout << "not found\n";
        std::cout << "Search steps: " << table.last_probe_count() << '\n';
        std::cout << "Duplicate keys skipped: " << duplicates << '\n';

        // Exercise deletion using full-record equality before invoking the
        // key-based map removal operation.
        if (!loaded.empty()) {
            const Entry candidate = loaded.front();
            Entry* current = table.get(key_of(candidate));
            if (current && same_entry(*current, candidate)) table.remove(key_of(candidate));
            Entry absent = candidate;
            absent.id = -1;
            if (table.get(key_of(absent))) table.remove(key_of(absent));
        }

        std::ofstream output("output_hash.txt");
        if (!output) throw std::runtime_error("cannot create output_hash.txt");
        table.print(output);
        if (!output) throw std::runtime_error("failed writing output_hash.txt");
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
