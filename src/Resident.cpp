#include "Resident.h"
#include "Debug.h"

Resident::Resident()
    : Person(), familyId(0), campId(0), shelterLabel("") {
    RELIEF_LIFECYCLE("Resident created #" << getId());
}

// The Person(...) call here is used only when Resident is the most-derived class. For a ResidentVolunteer it is ignored, because a virtual base is initialised by the most-derived constructor.
Resident::Resident(const std::string& name, int age, char sex, int familyId, int campId)
    : Person(name, age, sex), familyId(familyId), campId(campId), shelterLabel("") {
    RELIEF_LIFECYCLE("Resident created #" << getId());
}

Resident::Resident(const Resident& other)
    : Person(other), familyId(other.familyId), campId(other.campId),
      shelterLabel(other.shelterLabel) {
    RELIEF_LIFECYCLE("Resident copied  #" << getId());
}

Resident::~Resident() {
    RELIEF_LIFECYCLE("Resident destroyed #" << getId());
}

int Resident::getFamilyId() const { return familyId; }
int Resident::getCampId() const { return campId; }
const std::string& Resident::getShelterLabel() const { return shelterLabel; }
void Resident::setFamilyId(int newFamilyId) { familyId = newFamilyId; }

void Resident::assignTo(int newCampId, const std::string& newShelterLabel) {
    campId = newCampId;
    shelterLabel = newShelterLabel;
}
