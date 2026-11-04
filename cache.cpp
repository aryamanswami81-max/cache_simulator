#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <string>
#include <algorithm>
using namespace std;

struct Ref { char rw; int address; };
struct Line {
    bool valid = false;
    bool dirty = false;
    int tag = -1;
    int block = -1;
    long long inserted = 0;
    long long last_used = 0;
};

enum Mapping { DIRECT = 1, FULL = 2, SET_ASSOC = 3 };
enum Policy { FIFO = 1, LRU = 2, MRU = 3, RANDOM = 4 };
enum WritePolicy { WRITE_THROUGH = 1, WRITE_BACK = 2 };

bool is_power_of_two(int x) { return x > 0 && (x & (x - 1)) == 0; }
bool is_read(char c) { return c == 'R' || c == 'r'; }
bool is_write(char c) { return c == 'W' || c == 'w'; }

int choose_victim(const vector<Line>& cache, int start, int ways, Policy policy) {
    for (int w = 0; w < ways; ++w)
        if (!cache[start + w].valid) return w;
    int victim = 0;
    if (policy == FIFO) {
        for (int w = 1; w < ways; ++w)
            if (cache[start + w].inserted < cache[start + victim].inserted) victim = w;
    } else if (policy == LRU) {
        for (int w = 1; w < ways; ++w)
            if (cache[start + w].last_used < cache[start + victim].last_used) victim = w;
    } else if (policy == MRU) {
        for (int w = 1; w < ways; ++w)
            if (cache[start + w].last_used > cache[start + victim].last_used) victim = w;
    } else {
        victim = rand() % ways;
    }
    return victim;
}

bool shadow_access(vector<Line>& shadow, int block, long long time) {
    for (Line& line : shadow) {
        if (line.valid && line.block == block) {
            line.last_used = time;
            return true;
        }
    }
    int victim = 0;
    for (int i = 0; i < (int)shadow.size(); ++i) {
        if (!shadow[i].valid) { victim = i; break; }
        if (shadow[i].last_used < shadow[victim].last_used) victim = i;
    }
    shadow[victim].valid = true;
    shadow[victim].block = block;
    shadow[victim].last_used = time;
    return false;
}

bool read_trace_file(const string& filename, vector<Ref>& trace) {
    ifstream file(filename);
    if (!file) return false;
    char op; int address;
    while (file >> op >> address) {
        if ((!is_read(op) && !is_write(op)) || address < 0) return false;
        trace.push_back({(char)(is_write(op) ? 'W' : 'R'), address});
    }
    return !trace.empty();
}

bool get_trace(vector<Ref>& trace, int memory_size) {
    int source;
    cout << "Trace source (1=file, 2=typed input, 3=sequential, 4=looping, 5=random): ";
    if (!(cin >> source)) return false;
    if (source == 1) {
        string filename;
        cout << "Trace filename (e.g. trace.txt): "; cin >> filename;
        if (!read_trace_file(filename, trace)) {
            cout << "Could not read trace. Use lines like R 0 or W 16.\n";
            return false;
        }
    } else if (source == 2) {
        int n;
        cout << "Number of references: "; cin >> n;
        if (!cin || n <= 0) return false;
        cout << "Enter each reference as R address or W address:\n";
        for (int i = 0; i < n; ++i) {
            char op; int address; cin >> op >> address;
            if (!cin || (!is_read(op) && !is_write(op)) || address < 0 || address >= memory_size) {
                cout << "Invalid operation/address.\n"; return false;
            }
            trace.push_back({(char)(is_write(op) ? 'W' : 'R'), address});
        }
    } else if (source == 3) {
        int n, start;
        cout << "Number of references: "; cin >> n;
        cout << "Starting address: "; cin >> start;
        if (!cin || n <= 0 || start < 0 || start >= memory_size || (long long)start + n - 1 >= memory_size) {
            cout << "Invalid sequential trace range.\n"; return false;
        }
        for (int i = 0; i < n; ++i) trace.push_back({'R', start + i});
    } else if (source == 4) {
        int n, length;
        cout << "Number of references: "; cin >> n;
        cout << "Loop length (number of distinct addresses): "; cin >> length;
        if (!cin || n <= 0 || length <= 0 || length > memory_size) {
            cout << "Invalid count or loop length.\n"; return false;
        }
        for (int i = 0; i < n; ++i) trace.push_back({'R', i % length});
    } else if (source == 5) {
        int n, range;
        cout << "Number of references: "; cin >> n;
        cout << "Address range size (0 = full memory): "; cin >> range;
        if (!cin || n <= 0) return false;
        if (range <= 0 || range > memory_size) range = memory_size;
        for (int i = 0; i < n; ++i) {
            int address = rand() % range;
            char op = (rand() % 4 == 0) ? 'W' : 'R';
            trace.push_back({op, address});
        }
    } else {
        cout << "Invalid trace source.\n"; return false;
    }
    for (const Ref& r : trace) {
        if (r.address < 0 || r.address >= memory_size) {
            cout << "Address " << r.address << " is outside main memory.\n";
            return false;
        }
    }
    return !trace.empty();
}

void run_simulator(int memory_size, int cache_size, int block_size, int default_ways) {
    int mapping_input;
    cout << "Mapping (1=Direct, 2=Fully associative, 3=Set associative): ";
    cin >> mapping_input;
    if (!cin || mapping_input < 1 || mapping_input > 3) { cout << "Invalid mapping.\n"; return; }
    Mapping mapping = (Mapping)mapping_input;

    int lines = cache_size / block_size;
    int ways = 1;
    if (mapping == FULL) ways = lines;
    else if (mapping == SET_ASSOC) {
        cout << "Associativity in ways (default " << default_ways << "): ";
        cin >> ways;
        if (!cin || ways <= 0 || lines % ways != 0) { cout << "Invalid associativity; it must divide the cache line count.\n"; return; }
    }
    int sets = lines / ways;
    if (mapping == DIRECT) { ways = 1; sets = lines; }
    if (mapping == FULL) { ways = lines; sets = 1; }

    int policy_input;
    cout << "Replacement (1=FIFO, 2=LRU, 3=MRU, 4=Random): "; cin >> policy_input;
    if (!cin || policy_input < 1 || policy_input > 4) { cout << "Invalid replacement policy.\n"; return; }
    Policy policy = (Policy)policy_input;

    int write_policy_input;
    cout << "Write policy (1=Write-through, 2=Write-back): ";
    cin >> write_policy_input;
    if (!cin || (write_policy_input != 1 && write_policy_input != 2)) {
        cout << "Invalid write policy.\n";
        return;
    }
    WritePolicy write_policy = (WritePolicy)write_policy_input;

    double hit_time, miss_penalty;
    cout << "Hit time (cycles): "; cin >> hit_time;
    cout << "Additional miss penalty (cycles): "; cin >> miss_penalty;
    if (!cin || hit_time < 0 || miss_penalty < 0) { cout << "Times must be non-negative.\n"; return; }

    vector<Ref> trace;
    if (!get_trace(trace, memory_size)) return;

    vector<Line> cache(lines), shadow(lines);
    vector<bool> wb_dirty(lines, false);
    vector<bool> seen((size_t)(memory_size / block_size), false);
    long long hits = 0, misses = 0, compulsory = 0, capacity = 0, conflict = 0;
    long long write_through_writes = 0, write_back_evictions = 0, time = 0;

    cout << "\nNo\tRW\tAddress\tBlock\tTag\tSet\tOffset\tResult\tMiss type\n";
    for (size_t i = 0; i < trace.size(); ++i) {
        ++time;
        const int address = trace[i].address;
        const int block = address / block_size;
        const int offset = address % block_size;
        const int set_index = block % sets;
        const int tag = block / sets;
        const int start = set_index * ways;
        const bool write = is_write(trace[i].rw);
        const bool shadow_hit = shadow_access(shadow, block, time);
        if (write) ++write_through_writes;

        int present = -1;
        for (int w = 0; w < ways; ++w) {
            Line& line = cache[start + w];
            if (line.valid && line.tag == tag) { present = start + w; break; }
        }

        string miss_type = "-";
        if (present >= 0) {
            ++hits;
            cache[present].last_used = time;
            if (write) {
                wb_dirty[present] = true;
                cache[present].dirty = (write_policy == WRITE_BACK);
            }
        } else {
            ++misses;
            if (!seen[block]) { ++compulsory; miss_type = "Compulsory"; }
            else if (shadow_hit) { ++conflict; miss_type = "Conflict"; }
            else { ++capacity; miss_type = "Capacity"; }

            int place = start + choose_victim(cache, start, ways, policy);
            if (cache[place].valid && wb_dirty[place]) ++write_back_evictions;
            wb_dirty[place] = write;

            cache[place].valid = true;
            cache[place].tag = tag;
            cache[place].block = block;
            cache[place].inserted = time;
            cache[place].last_used = time;
            cache[place].dirty = (write_policy == WRITE_BACK && write);
        }
        seen[block] = true;
        cout << i + 1 << '\t' << trace[i].rw << '\t' << address << '\t' << block << '\t'
             << tag << '\t' << set_index << '\t' << offset << '\t'
             << (present >= 0 ? "HIT" : "MISS") << '\t' << miss_type << '\n';
    }

    const double miss_ratio = (double)misses / trace.size();
    const double hit_ratio = (double)hits / trace.size();
    const double amat = hit_time + miss_ratio * miss_penalty;
    int dirty_left = 0;
    int wb_dirty_left = 0;
    for (size_t i = 0; i < cache.size(); ++i) {
        if (cache[i].valid && cache[i].dirty) ++dirty_left;
        if (cache[i].valid && wb_dirty[i]) ++wb_dirty_left;
    }
    const long long write_back_total = write_back_evictions + wb_dirty_left;

    cout << "\nTotal references: " << trace.size() << '\n'
         << "Hits: " << hits << '\n'
         << "Misses: " << misses << '\n'
         << "Hit ratio: " << hit_ratio << '\n'
         << "Miss ratio: " << miss_ratio << '\n'
         << "\nCompulsory misses: " << compulsory << '\n'
         << "Capacity misses: " << capacity << '\n'
         << "Conflict misses: " << conflict << '\n'
         << "AMAT = hit time + miss ratio * additional miss penalty = " << amat << " cycles\n"
         << "\nWrite-policy comparison for this same trace (write-allocate assumed):\n"
         << "Write-through actual memory writes (one per store): " << write_through_writes << '\n'
         << "Write-back writes on dirty evictions: " << write_back_evictions << '\n'
         << "Write-back dirty blocks remaining before flush: " << wb_dirty_left << '\n'
         << "Write-back final-flush writes: " << wb_dirty_left << '\n'
         << "Write-back total actual memory writes (evictions + final flush): " << write_back_total << '\n'
         << "Selected write policy: " << (write_policy == WRITE_THROUGH ? "Write-through" : "Write-back") << '\n'
         << "Selected-policy actual memory writes: "
         << (write_policy == WRITE_THROUGH ? write_through_writes : write_back_total) << '\n'
         << "Dirty lines in selected cache before final flush: " << dirty_left << '\n';

    cout << "\nFinal cache contents\nSet\tWay\tTag\tBlock\tDirty\n";
    for (int s = 0; s < sets; ++s) {
        for (int w = 0; w < ways; ++w) {
            const Line& line = cache[s * ways + w];
            if (line.valid)
                cout << s << '\t' << w << '\t' << line.tag << '\t' << line.block << '\t'
                     << (line.dirty ? "D" : "-") << '\n';
        }
    }
}

void run_basic_mapping(int mapping, int memory_size, int cache_size, int block_size, int default_ways) {
    int ways = 1;
    if (mapping == 2) ways = cache_size / block_size;
    if (mapping == 3) {
        cout << "Associativity in ways (default " << default_ways << "): "; cin >> ways;
        int lines = cache_size / block_size;
        if (!cin || ways <= 0 || lines % ways != 0) { cout << "Invalid associativity.\n"; return; }
    }
    int n, start;
    cout << "Number of sequential byte addresses: "; cin >> n;
    cout << "Starting address: "; cin >> start;
    if (!cin || n <= 0 || start < 0 || start >= memory_size || (long long)start + n - 1 >= memory_size) {
        cout << "Invalid range.\n"; return;
    }
    vector<Ref> trace;
    for (int i = 0; i < n; ++i) trace.push_back({'R', start + i});
    int lines = cache_size / block_size;
    int sets = lines / ways;
    if (mapping == 1) { ways = 1; sets = lines; }
    if (mapping == 2) { ways = lines; sets = 1; }
    vector<Line> cache(lines);
    int hits = 0, misses = 0;
    cout << "No\tAddress\tTag\tSet\tOffset\tResult\n";
    for (int i = 0; i < n; ++i) {
        int address = trace[i].address, block = address / block_size;
        int set_index = block % sets, tag = block / sets, offset = address % block_size;
        int present = -1, start_line = set_index * ways;
        for (int w = 0; w < ways; ++w)
            if (cache[start_line + w].valid && cache[start_line + w].tag == tag) { present = start_line + w; break; }
        if (present >= 0) ++hits;
        else {
            ++misses;
            int place = start_line;
            for (int w = 0; w < ways; ++w) if (!cache[start_line + w].valid) { place = start_line + w; break; }
            cache[place].valid = true; cache[place].tag = tag; cache[place].block = block;
        }
        cout << i + 1 << '\t' << address << '\t' << tag << '\t' << set_index << '\t' << offset << '\t'
             << (present >= 0 ? "HIT" : "MISS") << '\n';
    }
    cout << "Total references: " << n << "\nHits: " << hits << "\nMisses: " << misses
         << "\nHit ratio: " << (double)hits / n << "\nMiss ratio: " << (double)misses / n << '\n';
}

int main() {
    srand(42);
    int memory_size, cache_size, block_size, ways;
    int roll;
    cout << "Enter roll number (final digits; enter 0 for manual configuration): ";
    cin >> roll;
    if (!cin) { cout << "Invalid roll number.\n"; return 1; }

    if (roll != 0) {
        if (roll < 0) { cout << "Roll number must be non-negative.\n"; return 1; }
        block_size = 1 << (4 + roll % 3);
        int sets = 1 << (8 + roll % 3);
        ways = 1 << (1 + roll % 2);
        cache_size = sets * ways * block_size;
        long long mem = (long long)cache_size * (1LL << (10 + roll % 3));
        if (mem > numeric_limits<int>::max()) { cout << "Generated memory size is too large.\n"; return 1; }
        memory_size = (int)mem;
    } else {
        cout << "Main memory size in bytes: "; cin >> memory_size;
        cout << "Cache size in bytes: "; cin >> cache_size;
        cout << "Block size in bytes: "; cin >> block_size;
        cout << "Default associativity (ways): "; cin >> ways;
    }
    if (!cin || !is_power_of_two(memory_size) || !is_power_of_two(cache_size) ||
        !is_power_of_two(block_size) || !is_power_of_two(ways) ||
        cache_size > memory_size || cache_size % block_size != 0 ||
        block_size > cache_size || ways > cache_size / block_size ||
        (cache_size / block_size) % ways != 0) {
        cout << "Invalid configuration. Sizes and ways must be powers of two; block <= cache <= memory; ways must divide cache lines.\n";
        return 1;
    }
    cout << "\nConfiguration\nMain memory: " << memory_size << " bytes\nCache: " << cache_size
         << " bytes\nBlock: " << block_size << " bytes\nCache lines: " << cache_size / block_size
         << "\nDefault associativity: " << ways << " ways\n";

    int choice;
    cout << "\nMenu: 1=Direct mapping demo, 2=Fully associative demo, 3=Set-associative demo, 4=Integrated simulator\nChoice: ";
    cin >> choice;
    if (!cin) { cout << "Invalid menu input.\n"; return 1; }
    if (choice >= 1 && choice <= 3) run_basic_mapping(choice, memory_size, cache_size, block_size, ways);
    else if (choice == 4) run_simulator(memory_size, cache_size, block_size, ways);
    else { cout << "Invalid menu choice.\n"; return 1; }
    return 0;
}
