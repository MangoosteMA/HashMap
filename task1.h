#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iomanip>
#include <list>
#include <vector>

template<typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>>
class HashMap {
public:
    HashMap(Hash hasher = Hash());
    template<typename IteratorType>
    HashMap(IteratorType begin, IteratorType end, Hash hasher = Hash());
    HashMap(const std::initializer_list<std::pair<KeyType, ValueType>> &data, Hash hasher = Hash());
    HashMap(const HashMap<KeyType, ValueType, Hash> &another);

    HashMap& operator=(const HashMap<KeyType, ValueType, Hash> &another);

    size_t size() const;
    bool empty() const;
    Hash hash_function() const;

    void insert(const std::pair<KeyType, ValueType> &new_value);
    void erase(const KeyType &key);

    using ElementsContainerType = std::list<std::pair<const KeyType, ValueType>>;
    using iterator = typename ElementsContainerType::iterator;
    using const_iterator = typename ElementsContainerType::const_iterator;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    iterator find(const KeyType &key);
    const_iterator find(const KeyType &key) const;
    ValueType& operator[](const KeyType &key);
    const ValueType& at(const KeyType &key) const;
    void clear();

private:
    struct HashTableData {
        bool is_empty = true;
        size_t distance = 0;
        typename ElementsContainerType::iterator iterator;
    };

    inline static size_t REBUILD_CONSTANT = 3;
    inline static size_t EXTENDED_CONSTANT = 2;
    inline static size_t EXTENDED_SHIFT = 3;

    Hash hasher_;
    std::vector<HashTableData> data_;
    size_t size_;
    ElementsContainerType elemets_;

    void reserve_(size_t size);
    void rebuild_();
    void next_(size_t &positions) const;
};

template<typename KeyType, typename ValueType, typename Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(Hash hasher) : hasher_(hasher), size_(0) {}

template<typename KeyType, typename ValueType, typename Hash>
template<typename IteratorType>
HashMap<KeyType, ValueType, Hash>::HashMap(IteratorType begin, IteratorType end, Hash hasher)
: hasher_(hasher), size_(0) {
    reserve_(std::distance(begin, end));
    for (auto it = begin; it != end; ++it) {
        insert(*it);
    }
}

template<typename KeyType, typename ValueType, typename Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(const std::initializer_list<std::pair<KeyType, ValueType>> &data,
                                           Hash hasher)
: hasher_(hasher), size_(0) {
    reserve_(data.size());
    for (auto &value : data) {
        insert(value);
    }
}

template<typename KeyType, typename ValueType, typename Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(const HashMap<KeyType, ValueType, Hash> &another)
: hasher_(another.hasher_), size_(0) {
    reserve_(another.size());
    for (auto &value : another) {
        insert(value);
    }
}

template<typename KeyType, typename ValueType, typename Hash>
HashMap<KeyType, ValueType, Hash>&
        HashMap<KeyType, ValueType, Hash>::operator=(const HashMap<KeyType, ValueType, Hash> &another) {
    if (this == &another) {
        return *this;
    }
    hasher_ = another.hasher_;
    reserve_(another.size());
    for (auto &value : another) {
        insert(value);
    }
    return *this;
}

template<typename KeyType, typename ValueType, typename Hash>
size_t HashMap<KeyType, ValueType, Hash>::size() const {
    return size_;
}

template<typename KeyType, typename ValueType, typename Hash>
bool HashMap<KeyType, ValueType, Hash>::empty() const {
    return size_ == 0;
}

template<typename KeyType, typename ValueType, typename Hash>
Hash HashMap<KeyType, ValueType, Hash>::hash_function() const {
    return hasher_;
}

template<typename KeyType, typename ValueType, typename Hash>
void HashMap<KeyType, ValueType, Hash>::insert(const std::pair<KeyType, ValueType> &new_value) {
    size_++;
    elemets_.emplace_back(new_value);
    if (size_ * REBUILD_CONSTANT > data_.size()) {
        data_ = std::vector<HashTableData>(EXTENDED_CONSTANT * data_.size() + EXTENDED_SHIFT);
        rebuild_();
    } else {
        size_t position = hasher_(new_value.first) % data_.size(), distance = 0;
        iterator current_elemet = std::prev(elemets_.end());
        while (true) {
            if (data_[position].is_empty) {
                data_[position].is_empty = false;
                data_[position].distance = distance;
                data_[position].iterator = current_elemet;
                return;
            }
            if (data_[position].iterator->first == current_elemet->first) {
                return;
            }
            if (data_[position].distance < distance) {
                std::swap(data_[position].iterator, current_elemet);
                std::swap(data_[position].distance, distance);
            }
            ++distance;
            next_(position);
        }
    }
}

template<typename KeyType, typename ValueType, typename Hash>
void HashMap<KeyType, ValueType, Hash>::erase(const KeyType &key) {
    if (size_ == 0) {
        return;
    }
    size_t position = hasher_(key) % data_.size(), distance = 0;
    while (true) {
        if (data_[position].is_empty) {
            return;
        }
        if (data_[position].distance == distance && data_[position].iterator->first == key) {
            size_--;
            elemets_.erase(data_[position].iterator);
            data_[position].is_empty = true;
            size_t prev_position = position;
            next_(position);
            while (!data_[position].is_empty && data_[position].distance > 0) {
                data_[prev_position].is_empty = false;
                data_[prev_position].distance = data_[position].distance - 1;
                data_[prev_position].iterator = data_[position].iterator;
                data_[position].is_empty = true;
                prev_position = position;
                next_(position);
            }
            return;
        }
        ++distance;
        next_(position);
    }
}

template<typename KeyType, typename ValueType, typename Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::begin() {
    return elemets_.begin();
}

template<typename KeyType, typename ValueType, typename Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::end() {
    return elemets_.end();
}

template<typename KeyType, typename ValueType, typename Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::begin() const {
    return elemets_.begin();
}

template<typename KeyType, typename ValueType, typename Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::end() const {
    return elemets_.end();
}

template<typename KeyType, typename ValueType, typename Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::find(const KeyType &key) {
    if (size_ == 0) {
        return end();
    }
    size_t position = hasher_(key) % data_.size(), distance = 0;
    while (true) {
        if (data_[position].is_empty) {
            return end();
        }
        if (data_[position].distance == distance && data_[position].iterator->first == key) {
            return data_[position].iterator;
        }
        ++distance;
        next_(position);
    }
}

template<typename KeyType, typename ValueType, typename Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::find(const KeyType &key) const {
    if (size_ == 0) {
        return end();
    }
    size_t position = hasher_(key) % data_.size(), distance = 0;
    while (true) {
        if (data_[position].is_empty) {
            return end();
        }
        if (data_[position].distance == distance && data_[position].iterator->first == key) {
            return data_[position].iterator;
        }
        ++distance;
        next_(position);
    }
}

template<typename KeyType, typename ValueType, typename Hash>
ValueType& HashMap<KeyType, ValueType, Hash>::operator[](const KeyType &key) {
    auto it = find(key);
    if (it == end()) {
        insert({key, ValueType()});
        it = find(key);
    }
    return it->second;
}

template<typename KeyType, typename ValueType, typename Hash>
const ValueType& HashMap<KeyType, ValueType, Hash>::at(const KeyType &key) const {
    auto it = find(key);
    if (it == end()) {
        throw std::out_of_range("There is no such element in the map.");
    }
    return it->second;
}

template<typename KeyType, typename ValueType, typename Hash>
void HashMap<KeyType, ValueType, Hash>::clear() {
    for (auto &[key, value] : elemets_) {
        size_t position = hasher_(key) % data_.size();
        while (!data_[position].is_empty) {
            data_[position].is_empty = true;
            next_(position);
        }
    }
    size_ = 0;
    elemets_.clear();
}

template<typename KeyType, typename ValueType, typename Hash>
void HashMap<KeyType, ValueType, Hash>::reserve_(size_t size) {
    data_.resize(EXTENDED_CONSTANT * size);
}

template<typename KeyType, typename ValueType, typename Hash>
void HashMap<KeyType, ValueType, Hash>::rebuild_() {
    size_ = 0;
    ElementsContainerType current_elements;
    current_elements.swap(elemets_);
    for (auto &value : current_elements) {
        insert(value);
    }
}

template<typename KeyType, typename ValueType, typename Hash>
void HashMap<KeyType, ValueType, Hash>::next_(size_t &position) const {
    ++position;
    if (position == data_.size()) {
        position = 0;
    }
}
