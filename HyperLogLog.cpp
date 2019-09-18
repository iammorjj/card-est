#include <set>
#include <random>
#include <cassert>

class HyperLogLogPlus {
private:
    const int numHashBitsForBucket = 11;
    const int numHashBitsForRank = 64 - numHashBitsForBucket;
    const int numOfBuckets = 1 << numHashBitsForBucket;

    const double biasCorrection = 0.7213 / (1 + 1.079 / numOfBuckets);

    uint8_t* buckets = new uint8_t[numOfBuckets]();

    uint64_t getHash(const uint64_t data) const {
        uint64_t x = data;
        x = (x ^ (x >> 30)) * (uint64_t)0xbf58476d1ce4e5b9;
        x = (x ^ (x >> 27)) * (uint64_t)0x94d049bb133111eb;
        x = x ^ (x >> 31);
        return x;
    }

    void add(const uint64_t hash) {
        size_t ind = hash >> numHashBitsForRank;
        buckets[ind] = std::max(buckets[ind], getRank(hash, numHashBitsForRank));
    }

    uint8_t getRank(const uint64_t hash, const int len) const {
        uint64_t x = hash;
        uint8_t r = 1;
        while ((x & 1) == 0 && r <= len) { ++r; x >>= 1; }
        return r;
    }

    int calculate() const {
        double c = 0.0;
        for (int i = 0; i < numOfBuckets; ++i)
            c += 1.0 / (1L << buckets[i]);

        double E = biasCorrection * numOfBuckets * numOfBuckets / c;
        if (E <= 5.0/2 * numOfBuckets) {
            int V = 0;
            for (int i = 0; i < numOfBuckets; ++i)
                if (buckets[i] == 0)
                    ++V;
            if (V > 0)
                E = numOfBuckets * log((double)numOfBuckets / V);
        }

        return (int)E;
    }

public:
    ~HyperLogLogPlus() {
        delete[] buckets;
    }
    void add(int x) {
        add(getHash(x));
    }
    int getEstimate() const {
        return calculate();
    }
};

class UniqCounter {

    HyperLogLogPlus obj;

public:
    void add(int x) {
        obj.add(x);
    }

    int get_uniq_num() const {
        return obj.getEstimate();
    }
};

double relative_error(int expected, int got) {
    return abs(got - expected) / (double) expected;
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());

    const int N = (int) 1e6;
    for (int k : {1, 10, 1000, 10000, N / 10, N, N * 10}) {
        std::uniform_int_distribution<> dis(1, k);
        std::set<int> all;
        UniqCounter counter;
        for (int i = 0; i < N; i++) {
            int value = dis(gen);
            all.insert(value);
            counter.add(value);
        }
        int expected = (int) all.size();
        int counter_result = counter.get_uniq_num();
        double error = relative_error(expected, counter_result);
        printf("%d numbers in range [1 .. %d], %d uniq, %d result, %.5f relative error\n", N, k, expected, counter_result, error);
        assert(error <= 0.1);
    }

    return 0;
}
