#ifndef RELIEF_CAMP_H
#define RELIEF_CAMP_H

#include "Stock.h"
#include <string>
#include <vector>
#include <ostream>

class Family;            // admit/release only need a reference
class ReportGenerator;   // declared a friend below

// A relief camp: a school, a community hall or a cluster of tents on an
// embankment, with a capacity, a set of shelters and its own stock.
class Camp {
public:
    // [Nested class] A shelter has no meaning outside the camp that contains
    // it, so its name lives inside Camp: `Camp::Shelter`.
    class Shelter {
        std::string label;
        int capacity;
        int occupancy;
    public:
        Shelter();
        Shelter(const std::string& label, int capacity);
        const std::string& getLabel() const;
        int getCapacity() const;
        int getOccupancy() const;
        int freeSpace() const;
        bool hasRoom(int people) const;
        void occupy(int people);        // throws OverCapacityException
        void vacate(int people);        // clamps at zero
        void setOccupancy(int people);  // CSV loading
        friend std::ostream& operator<<(std::ostream& os, const Shelter& s);
    };

private:
    static int nextId;                  // [Static data member]

    int id;
    std::string name;
    std::string district;
    int capacity;
    // The authoritative head-count for the camp. A Shelter's own occupancy is
    // only the placement detail within it. A caller that admits a family must
    // occupy the chosen shelter with the same number of people, and a caller
    // that releases must vacate it. The two counters are kept separate on purpose.
    int occupancy;
    std::vector<Shelter> shelters;
    Stock stock;

public:
    Camp();
    Camp(const std::string& name, const std::string& district, int capacity);
    Camp(const Camp& other);
    ~Camp();

    static int generateId();            // [Static member function]
    static void resetIdCounter(int startAt);
    void adoptId(int existingId);

    int getId() const;
    const std::string& getName() const;
    const std::string& getDistrict() const;
    int getCapacity() const;
    int getOccupancy() const;
    int freeSpace() const;
    bool isOverflowing() const;         // true once the camp is full or beyond
    double occupancyRate() const;

    // [Objects as arguments] the camp is told about a whole family at once
    void admit(const Family& f);        // throws OverCapacityException
    void release(const Family& f);

    // [Unary operators] one more, or one fewer, person in the camp
    Camp& operator++();                 // prefix: ++camp
    Camp operator++(int);               // postfix: camp++
    Camp& operator--();                 // prefix
    Camp operator--(int);               // postfix

    void addShelter(const Shelter& s);
    int shelterCount() const;
    const Shelter& shelterAt(int index) const;   // throws std::out_of_range
    Shelter& shelterAt(int index);

    Stock& stockRef();
    const Stock& getStock() const;
    void setOccupancy(int people);      // CSV loading
    void setName(const std::string& newName);

    // [Friend class] ReportGenerator reads the camp's private members
    // (shelters, occupancy, stock) directly instead of going through the
    // public accessors, and `friend` is what permits that.
    friend class ReportGenerator;

    friend std::ostream& operator<<(std::ostream& os, const Camp& c);
};

#endif
