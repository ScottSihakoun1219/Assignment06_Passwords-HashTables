#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

uint64_t demoHash(const std::string& s) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) {
        h = (h ^ c) * 1099511628211ULL;
    }
    return h;
}

const std::string PEPPER = "CS210Pepper";

struct UserRecord {
    std::string username;
    std::string salt;
    uint64_t passwordHash;

    UserRecord() {
        username = "";
        salt = "";
        passwordHash = 0;
    }

    UserRecord(std::string u, std::string s, uint64_t h) {
        username = u;
        salt = s;
        passwordHash = h;
    }
};

uint64_t hashPassword(const std::string& salt, const std::string& password) {
    return demoHash(salt + password + PEPPER);
}

bool isPrimeNumber(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

int nextPrimeNumber(int n) {
    while (!isPrimeNumber(n)) {
        n++;
    }
    return n;
}

// Separate Chaining
struct ChainNode {
    UserRecord user;
    ChainNode* next;

    ChainNode(UserRecord u) {
        user = u;
        next = nullptr;
    }
};

class ChainingTable {
private:
    std::vector<ChainNode*> buckets;
    int capacity;
    int count;
    int collisionCount;
    int rehashCount;

    int bucketFor(const std::string& key) const {
        return demoHash(key) % capacity;
    }

    void rehash() {
        int oldCapacity = capacity;
        capacity = capacity * 2;
        std::vector<ChainNode*> newBuckets(capacity, nullptr);

        for (int i = 0; i < oldCapacity; i++) {
            ChainNode* current = buckets[i];
            while (current != nullptr) {
                ChainNode* nextNode = current->next;
                int newIndex = demoHash(current->user.username) % capacity;
                current->next = newBuckets[newIndex];
                newBuckets[newIndex] = current;
                current = nextNode;
            }
        }
        buckets = newBuckets;
        rehashCount++;
    }

public:
    ChainingTable(int startCapacity) {
        capacity = startCapacity;
        count = 0;
        collisionCount = 0;
        rehashCount = 0;
        buckets.resize(capacity, nullptr);
    }

    double loadFactor() const {
        return (double)count / capacity;
    }

    bool insert(const UserRecord& user) {
        if (find(user.username) != nullptr) return false;
        if ((double)(count + 1) / capacity > 0.75) rehash();

        int index = bucketFor(user.username);
        if (buckets[index] != nullptr) {
            collisionCount++;
        }

        ChainNode* newNode = new ChainNode(user);
        newNode->next = buckets[index];
        buckets[index] = newNode;
        count++;
        return true;
    }

    UserRecord* find(const std::string& username) {
        int index = bucketFor(username);
        ChainNode* current = buckets[index];
        while (current != nullptr) {
            if (current->user.username == username) {
                return &(current->user);
            }
            current = current->next;
        }
        return nullptr;
    }

    bool remove(const std::string& username) {
        int index = bucketFor(username);
        ChainNode* current = buckets[index];
        ChainNode* prev = nullptr;

        while (current != nullptr) {
            if (current->user.username == username) {
                if (prev == nullptr) {
                    buckets[index] = current->next;
                }
                else {
                    prev->next = current->next;
                }
                delete current;
                count--;
                return true;
            }
            prev = current;
            current = current->next;
        }
        return false;
    }

    int getCount() const { return count; }
    int getCapacity() const { return capacity; }
    int getCollisions() const { return collisionCount; }
    int getRehashes() const { return rehashCount; }

    ~ChainingTable() {
        for (int i = 0; i < capacity; i++) {
            ChainNode* current = buckets[i];
            while (current != nullptr) {
                ChainNode* temp = current;
                current = current->next;
                delete temp;
            }
        }
    }
};

// Linear Probing
const int SLOT_EMPTY = 0;
const int SLOT_FILLED = 1;
const int SLOT_GONE = 2;

class LinearProbeTable {
private:
    std::vector<UserRecord> slots;
    std::vector<int> state;
    int capacity;
    int count;
    int collisionCount;
    int rehashCount;

    int homeFor(const std::string& key) const {
        return demoHash(key) % capacity;
    }

    void rehash() {
        std::vector<UserRecord> oldSlots = slots;
        std::vector<int> oldState = state;
        int oldCapacity = capacity;

        capacity = capacity * 2;
        slots.assign(capacity, UserRecord());
        state.assign(capacity, SLOT_EMPTY);
        count = 0;

        for (int i = 0; i < oldCapacity; i++) {
            if (oldState[i] == SLOT_FILLED) {
                insert(oldSlots[i]);
            }
        }
        rehashCount++;
    }

public:
    LinearProbeTable(int startCapacity) {
        capacity = startCapacity;
        count = 0;
        collisionCount = 0;
        rehashCount = 0;
        slots.assign(capacity, UserRecord());
        state.assign(capacity, SLOT_EMPTY);
    }

    double loadFactor() const {
        return (double)count / capacity;
    }

    bool insert(const UserRecord& user) {
        if (find(user.username) != nullptr) return false;
        if ((double)(count + 1) / capacity > 0.5) rehash();

        int index = homeFor(user.username);
        bool sawCollision = false;

        while (state[index] == SLOT_FILLED) {
            sawCollision = true;
            index = (index + 1) % capacity;
        }

        if (sawCollision) collisionCount++;

        slots[index] = user;
        state[index] = SLOT_FILLED;
        count++;
        return true;
    }

    UserRecord* find(const std::string& username) {
        int index = homeFor(username);
        for (int i = 0; i < capacity; i++) {
            if (state[index] == SLOT_EMPTY) return nullptr;
            if (state[index] == SLOT_FILLED && slots[index].username == username) {
                return &slots[index];
            }
            index = (index + 1) % capacity;
        }
        return nullptr;
    }

    bool remove(const std::string& username) {
        int index = homeFor(username);
        for (int i = 0; i < capacity; i++) {
            if (state[index] == SLOT_EMPTY) return false;
            if (state[index] == SLOT_FILLED && slots[index].username == username) {
                state[index] = SLOT_GONE;
                count--;
                return true;
            }
            index = (index + 1) % capacity;
        }
        return false;
    }

    int getCount() const { return count; }
    int getCapacity() const { return capacity; }
    int getCollisions() const { return collisionCount; }
    int getRehashes() const { return rehashCount; }
};

// Quadratic Probing
class QuadraticProbeTable {
private:
    std::vector<UserRecord> slots;
    std::vector<int> state;
    int capacity;
    int count;
    int collisionCount;
    int rehashCount;

    int homeFor(const std::string& key) const {
        return demoHash(key) % capacity;
    }

    void rehash() {
        std::vector<UserRecord> oldSlots = slots;
        std::vector<int> oldState = state;
        int oldCapacity = capacity;

        capacity = nextPrimeNumber(capacity * 2);
        slots.assign(capacity, UserRecord());
        state.assign(capacity, SLOT_EMPTY);
        count = 0;

        for (int i = 0; i < oldCapacity; i++) {
            if (oldState[i] == SLOT_FILLED) {
                insert(oldSlots[i]);
            }
        }
        rehashCount++;
    }

public:
    QuadraticProbeTable(int startCapacity) {
        capacity = nextPrimeNumber(startCapacity);
        count = 0;
        collisionCount = 0;
        rehashCount = 0;
        slots.assign(capacity, UserRecord());
        state.assign(capacity, SLOT_EMPTY);
    }

    double loadFactor() const {
        return (double)count / capacity;
    }

    bool insert(const UserRecord& user) {
        if (find(user.username) != nullptr) return false;
        if ((double)(count + 1) / capacity > 0.5) rehash();

        int home = homeFor(user.username);
        int index = home;
        int step = 0;
        bool sawCollision = false;

        while (state[index] == SLOT_FILLED) {
            sawCollision = true;
            step++;
            index = (home + step * step) % capacity;
        }

        if (sawCollision) collisionCount++;

        slots[index] = user;
        state[index] = SLOT_FILLED;
        count++;
        return true;
    }

    UserRecord* find(const std::string& username) {
        int home = homeFor(username);
        for (int step = 0; step < capacity; step++) {
            int index = (home + step * step) % capacity;
            if (state[index] == SLOT_EMPTY) return nullptr;
            if (state[index] == SLOT_FILLED && slots[index].username == username) {
                return &slots[index];
            }
        }
        return nullptr;
    }

    bool remove(const std::string& username) {
        int home = homeFor(username);
        for (int step = 0; step < capacity; step++) {
            int index = (home + step * step) % capacity;
            if (state[index] == SLOT_EMPTY) return false;
            if (state[index] == SLOT_FILLED && slots[index].username == username) {
                state[index] = SLOT_GONE;
                count--;
                return true;
            }
        }
        return false;
    }

    int getCount() const { return count; }
    int getCapacity() const { return capacity; }
    int getCollisions() const { return collisionCount; }
    int getRehashes() const { return rehashCount; }
};

// Double Hashing
class DoubleHashTable {
private:
    std::vector<UserRecord> slots;
    std::vector<int> state;
    int capacity;
    int count;
    int collisionCount;
    int rehashCount;

    int homeFor(const std::string& key) const {
        return demoHash(key) % capacity;
    }

    int stepFor(const std::string& key) const {
        return 1 + (demoHash(key + "#step") % (capacity - 1));
    }

    void rehash() {
        std::vector<UserRecord> oldSlots = slots;
        std::vector<int> oldState = state;
        int oldCapacity = capacity;

        capacity = nextPrimeNumber(capacity * 2);
        slots.assign(capacity, UserRecord());
        state.assign(capacity, SLOT_EMPTY);
        count = 0;

        for (int i = 0; i < oldCapacity; i++) {
            if (oldState[i] == SLOT_FILLED) {
                insert(oldSlots[i]);
            }
        }
        rehashCount++;
    }

public:
    DoubleHashTable(int startCapacity) {
        capacity = nextPrimeNumber(startCapacity);
        count = 0;
        collisionCount = 0;
        rehashCount = 0;
        slots.assign(capacity, UserRecord());
        state.assign(capacity, SLOT_EMPTY);
    }

    double loadFactor() const {
        return (double)count / capacity;
    }

    bool insert(const UserRecord& user) {
        if (find(user.username) != nullptr) return false;
        if ((double)(count + 1) / capacity > 0.5) rehash();

        int home = homeFor(user.username);
        int step = stepFor(user.username);
        int index = home;
        bool sawCollision = false;

        while (state[index] == SLOT_FILLED) {
            sawCollision = true;
            index = (index + step) % capacity;
        }

        if (sawCollision) collisionCount++;

        slots[index] = user;
        state[index] = SLOT_FILLED;
        count++;
        return true;
    }

    UserRecord* find(const std::string& username) {
        int home = homeFor(username);
        int step = stepFor(username);
        int index = home;

        for (int i = 0; i < capacity; i++) {
            if (state[index] == SLOT_EMPTY) return nullptr;
            if (state[index] == SLOT_FILLED && slots[index].username == username) {
                return &slots[index];
            }
            index = (index + step) % capacity;
        }
        return nullptr;
    }

    bool remove(const std::string& username) {
        int home = homeFor(username);
        int step = stepFor(username);
        int index = home;

        for (int i = 0; i < capacity; i++) {
            if (state[index] == SLOT_EMPTY) return false;
            if (state[index] == SLOT_FILLED && slots[index].username == username) {
                state[index] = SLOT_GONE;
                count--;
                return true;
            }
            index = (index + step) % capacity;
        }
        return false;
    }

    int getCount() const { return count; }
    int getCapacity() const { return capacity; }
    int getCollisions() const { return collisionCount; }
    int getRehashes() const { return rehashCount; }
};

bool verifyLogin(ChainingTable& table, const std::string& username, const std::string& password) {
    UserRecord* user = table.find(username);
    if (user == nullptr) {
        std::cout << "-> Login failed: unknown user '" << username << "'\n";
        return false;
    }

    uint64_t testHash = hashPassword(user->salt, password);
    if (testHash == user->passwordHash) {
        std::cout << "-> Login success! Welcome back, " << username << "!\n";
        return true;
    }
    else {
        std::cout << "-> Login failed: wrong password for '" << username << "'\n";
        return false;
    }
}

int main() {
    std::string testNames[] = { "ryan", "jason", "samantha", "selena", "sara", "jimmy" };
    std::string testPasswords[] = { "ryan2024", "jasonpass1", "samfit88", "selenadance", "sararocks5", "jimmyplay9" };
    std::string testSalts[] = { "saltR", "saltJ", "saltS1", "saltS2", "saltS3", "saltJ2" };
    int numUsers = 6;

    std::cout << "[1] Testing ChainingTable...\n";
    ChainingTable chainTable(5);
    for (int i = 0; i < numUsers; i++) {
        chainTable.insert(UserRecord(testNames[i], testSalts[i], hashPassword(testSalts[i], testPasswords[i])));
    }
    std::cout << "users: " << chainTable.getCount() << ", capacity: " << chainTable.getCapacity()
        << ", collisions: " << chainTable.getCollisions() << ", rehashes: " << chainTable.getRehashes() << "\n\n";

    std::cout << "[2] Testing LinearProbeTable...\n";
    LinearProbeTable linearTable(5);
    for (int i = 0; i < numUsers; i++) {
        linearTable.insert(UserRecord(testNames[i], testSalts[i], hashPassword(testSalts[i], testPasswords[i])));
    }
    std::cout << "users: " << linearTable.getCount() << ", capacity: " << linearTable.getCapacity()
        << ", collisions: " << linearTable.getCollisions() << ", rehashes: " << linearTable.getRehashes() << "\n\n";

    std::cout << "[3] Testing QuadraticProbeTable...\n";
    QuadraticProbeTable quadTable(5);
    for (int i = 0; i < numUsers; i++) {
        quadTable.insert(UserRecord(testNames[i], testSalts[i], hashPassword(testSalts[i], testPasswords[i])));
    }
    std::cout << "users: " << quadTable.getCount() << ", capacity: " << quadTable.getCapacity()
        << ", collisions: " << quadTable.getCollisions() << ", rehashes: " << quadTable.getRehashes() << "\n\n";

    std::cout << "[4] Testing DoubleHashTable...\n";
    DoubleHashTable doubleTable(5);
    for (int i = 0; i < numUsers; i++) {
        doubleTable.insert(UserRecord(testNames[i], testSalts[i], hashPassword(testSalts[i], testPasswords[i])));
    }
    std::cout << "users: " << doubleTable.getCount() << ", capacity: " << doubleTable.getCapacity()
        << ", collisions: " << doubleTable.getCollisions() << ", rehashes: " << doubleTable.getRehashes() << "\n\n";

    std::cout << "[5] Testing Login System (ChainingTable)...\n";
    ChainingTable userTable(3);

    userTable.insert(UserRecord("ryan", "saltR", hashPassword("saltR", "ryan2024")));
    userTable.insert(UserRecord("jason", "saltJ", hashPassword("saltJ", "jasonpass1")));
    userTable.insert(UserRecord("samantha", "saltS1", hashPassword("saltS1", "samfit88")));
    userTable.insert(UserRecord("selena", "saltS2", hashPassword("saltS2", "selenadance")));
    userTable.insert(UserRecord("sara", "saltS3", hashPassword("saltS3", "sararocks5")));

    verifyLogin(userTable, "ryan", "ryan2024");
    verifyLogin(userTable, "ryan", "wrongpassword");
    verifyLogin(userTable, "random_user", "bad_password");

    userTable.remove("jason");
    verifyLogin(userTable, "jason", "jasonpass1");

    userTable.insert(UserRecord("jimmy", "saltJ2", hashPassword("saltJ2", "jimmyplay9")));
    userTable.insert(UserRecord("john", "saltJ3", hashPassword("saltJ3", "johnwalks4")));
    userTable.insert(UserRecord("samuel", "saltS4", hashPassword("saltS4", "samuelrun7")));

    std::cout << "Final capacity: " << userTable.getCapacity()
        << ", Final load factor: " << userTable.loadFactor() << "\n";
    std::cout << "-> Login system tests completed successfully.\n";

    return 0;
}