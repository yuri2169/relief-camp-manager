#ifndef RELIEF_REGISTRY_H
#define RELIEF_REGISTRY_H

#include <vector>
#include <memory>

// One store for every kind of entity that has an ID. Camps,
// families and volunteers all need store / find / remove, so the code is
// written once and the compiler stamps out a version per type.
//
// T must provide `int getId() const`.
//
// Ownership note: this uses std::unique_ptr, while `Family` owns its
// members with raw new/delete. Both styles are in the project on purpose, so
// the manual version can be compared with the automatic one.
template <typename T>
class Registry {
    std::vector<std::unique_ptr<T> > items;

public:
    Registry() {}

    // unique_ptr cannot be copied, so neither can a Registry. Saying so out
    // loud is better than letting the compiler produce a confusing error.
    Registry(const Registry& other) = delete;
    Registry& operator=(const Registry& other) = delete;

    // Ownership of the item moves INTO the registry. The returned pointer is a
    // non-owning observer of the stored element (see the note above at()).
    T* add(std::unique_ptr<T> item) {
        T* raw = item.get();
        items.push_back(std::move(item));
        return raw;
    }

    // Finding does not change the registry
    T* findById(int id) const {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            if (items[i]->getId() == id) {
                return items[i].get();
            }
        }
        return 0;
    }

    bool remove(int id) {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            if (items[i]->getId() == id) {
                items.erase(items.begin() + i);
                return true;
            }
        }
        return false;
    }

    int size() const { return static_cast<int>(items.size()); }

    // Ownership rules for every pointer handed out by add(), at(), findById()
    // and all():
    //  - The registry owns each element through its std::unique_ptr.
    //  - A returned T* is a non-owning observer: use it, but NEVER delete it.
    //  - remove() and clear() destroy the element, so they invalidate a returned
    //    pointer and leave any all() snapshot stale. (Growing the vector does
    //    not: it moves the unique_ptr slots, not the objects they own.)
    T* at(int index) const {
        if (index < 0 || index >= static_cast<int>(items.size())) {
            return 0;
        }
        return items[index].get();
    }

    std::vector<T*> all() const {
        std::vector<T*> out;
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            out.push_back(items[i].get());
        }
        return out;
    }

    void clear() { items.clear(); }
};

#endif
