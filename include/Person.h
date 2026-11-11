#ifndef RELIEF_PERSON_H
#define RELIEF_PERSON_H

#include "Ration.h"
#include <string>
#include <ostream>

// [Abstract class] the root of the whole people hierarchy. It can never be
// instantiated: every concrete person must answer all five questions below.
class Person {
    // [Static data members] one counter shared by every Person ever created
    static int nextId;
    static int liveCount;

    int id;
    std::string name;
    int age;
    char sex;              // 'M', 'F' or 'O'
    bool pregnant;
    bool disabled;
    bool lactating;

protected:
    // Shared vulnerability bonus, used by every subclass's score.
    int flagBonus() const;

public:
    Person();                                              // [Default constructor]
    Person(const std::string& name, int age, char sex);    // [Parameterised constructor]
    Person(const Person& other);                           // [Copy constructor]
    virtual ~Person();                                     // [Virtual destructor]

    // [Static member functions]
    static int generateId();
    static int liveObjects();
    static void resetIdCounter(int startAt);
    static int lastIssuedId();

    // [Const member functions] every getter promises not to modify the object
    int getId() const;
    const std::string& getName() const;
    int getAge() const;
    char getSex() const;
    bool isPregnant() const;
    bool isDisabled() const;
    bool isLactating() const;

    void setFlags(bool pregnant, bool disabled, bool lactating);
    void setName(const std::string& name);
    void adoptId(int existingId);     // used when loading from CSV

    // [Pure virtual functions] -> runtime polymorphism
    virtual Ration rationNeed() const = 0;
    virtual int vulnerabilityScore() const = 0;
    virtual std::string describe() const = 0;
    virtual Person* clone() const = 0;          // deep copy through a base pointer
    virtual std::string typeTag() const = 0;    // short tag used by the CSV layer

    // [Friend function] printing is not a member, so it reads as `os << person`
    friend std::ostream& operator<<(std::ostream& os, const Person& p);
};

#endif
