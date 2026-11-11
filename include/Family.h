#ifndef RELIEF_FAMILY_H
#define RELIEF_FAMILY_H

#include "Person.h"
#include "Ration.h"
#include <string>
#include <ostream>

class RationCard;   // only the name is needed here; Family.cpp includes the definition

// A displaced household: the unit that gets registered, admitted and rationed.
//
// Ownership: a Family owns every Person in its members array. addMember takes
// ownership; memberAt hands out references that are observers only (never
// delete them, never keep them past the Family's lifetime or the next copy).
//
// [Dynamic memory]
// [Array of dynamic objects]
// Family owns its members through a raw Person** array that it grows itself.
// A std::vector<std::unique_ptr<Person>> would be the modern choice - Registry<T>
// shows that style - but owning raw memory here is what makes the
// rule of three below necessary and visible.
class Family {
    // [Static data member]
    static int nextId;

    int id;
    std::string headName;
    std::string village;
    int campId;                        // 0 means "not yet admitted"

    Person** members;                  // the dynamic array
    int count;                         // members in use
    int capacity;                      // slots allocated

    void grow();                       // doubles the array
    void copyFrom(const Family& other);// deep-copies another family's members
    void freeAll();                    // deletes every member and the array

public:
    // [Default constructor]
    Family();
    // [Parameterised constructor]
    Family(const std::string& headName, const std::string& village);
    // [Copy constructor]
    Family(const Family& other);
    // [Copy assignment operator]
    Family& operator=(const Family& other);
    // [Destructor]
    ~Family();

    // [Static member function]
    static int generateId();
    static void resetIdCounter(int startAt);
    void adoptId(int existingId);

    int getId() const;
    const std::string& getHeadName() const;
    const std::string& getVillage() const;
    int getCampId() const;
    void setCampId(int newCampId);
    void setHeadName(const std::string& newHeadName);

    int size() const;
    const Person& memberAt(int index) const;   // observer; throws std::out_of_range
    Person& memberAt(int index);               // observer; throws std::out_of_range
    void addMember(Person* p);                 // Family takes ownership of p
    int countOfTag(const std::string& tag) const;

    // [Runtime polymorphism]
    // The sum walks base pointers and each member answers with its own override.
    // [Returning objects]
    Ration dailyRation() const;
    int vulnerabilityScore() const;

    // Metric functions for [Pointer to member function] sorting through FamilySorter.
    // They all share one signature, which is the whole point.
    double metricVulnerability() const;
    double metricSize() const;
    double metricAverageAge() const;
    double metricInfantCount() const;

    // [Class to class conversion] a Family converts to the printable card an
    // official hands over. RationCard is only forward-declared above; the
    // definition lives in Family.cpp, which includes RationCard.h.
    operator RationCard() const;

    // [Friend functions]
    friend bool operator<(const Family& a, const Family& b);   // by vulnerability
    friend bool operator==(const Family& a, const Family& b);  // by identity
    friend std::ostream& operator<<(std::ostream& os, const Family& f);
};

#endif
