#include "TestRunner.h"
#include "Camp.h"
#include "Registry.h"
#include "Family.h"
#include "Adult.h"
#include "Infant.h"
#include "Exceptions.h"
#include <memory>
#include <stdexcept>
#include <sstream>

void run_camp_tests() {
    testing::beginSuite("Camp and the nested Shelter class");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    // --- the nested class on its own ---
    Camp::Shelter hall("Hall-A", 40);
    ASSERT_STR_EQ(hall.getLabel(), "Hall-A");
    ASSERT_EQ(hall.getCapacity(), 40);
    ASSERT_EQ(hall.getOccupancy(), 0);
    ASSERT_EQ(hall.freeSpace(), 40);
    ASSERT_TRUE(hall.hasRoom(40));
    ASSERT_FALSE(hall.hasRoom(41));

    hall.occupy(35);
    ASSERT_EQ(hall.getOccupancy(), 35);
    ASSERT_EQ(hall.freeSpace(), 5);
    ASSERT_THROWS(hall.occupy(6), OverCapacityException);
    ASSERT_EQ(hall.getOccupancy(), 35);         // unchanged after the throw
    hall.vacate(5);
    ASSERT_EQ(hall.getOccupancy(), 30);
    hall.vacate(1000);                          // clamps at zero rather than going negative
    ASSERT_EQ(hall.getOccupancy(), 0);

    // Guards on the setters and on negative arguments: rejected, and nothing changed.
    hall.occupy(10);
    ASSERT_THROWS(hall.occupy(-5), std::invalid_argument);
    ASSERT_EQ(hall.getOccupancy(), 10);
    ASSERT_THROWS(hall.vacate(-10), std::invalid_argument);
    ASSERT_EQ(hall.getOccupancy(), 10);
    ASSERT_THROWS(hall.setOccupancy(-1), std::invalid_argument);
    ASSERT_EQ(hall.getOccupancy(), 10);
    ASSERT_THROWS(hall.setOccupancy(41), OverCapacityException);
    ASSERT_EQ(hall.getOccupancy(), 10);
    hall.setOccupancy(40);                      // exactly full is allowed
    ASSERT_EQ(hall.getOccupancy(), 40);
    hall.vacate(40);

    Camp::Shelter blankShelter;
    ASSERT_EQ(blankShelter.getCapacity(), 0);

    // --- the camp ---
    Camp camp("Bhuragaon HS School", "Morigaon", 10);
    ASSERT_EQ(camp.getId(), 1);
    ASSERT_STR_EQ(camp.getName(), "Bhuragaon HS School");
    ASSERT_STR_EQ(camp.getDistrict(), "Morigaon");
    ASSERT_EQ(camp.getCapacity(), 10);
    ASSERT_EQ(camp.getOccupancy(), 0);
    ASSERT_EQ(camp.freeSpace(), 10);
    ASSERT_FALSE(camp.isOverflowing());
    ASSERT_NEAR(camp.occupancyRate(), 0.0, 1e-9);

    // Shelters live inside the camp.
    camp.addShelter(Camp::Shelter("Room-1", 6));
    camp.addShelter(Camp::Shelter("Tent-1", 4));
    ASSERT_EQ(camp.shelterCount(), 2);
    ASSERT_STR_EQ(camp.shelterAt(0).getLabel(), "Room-1");
    ASSERT_STR_EQ(camp.shelterAt(1).getLabel(), "Tent-1");
    ASSERT_THROWS(camp.shelterAt(2), std::out_of_range);

    // --- admitting a family ---
    Family big("Bipul Das", "Bhuragaon");
    for (int i = 0; i < 6; i++) {
        big.addMember(new Adult("Member " + std::to_string(i), 25 + i, 'M', big.getId(), 0));
    }
    camp.admit(big);
    ASSERT_EQ(camp.getOccupancy(), 6);
    ASSERT_EQ(camp.freeSpace(), 4);
    ASSERT_NEAR(camp.occupancyRate(), 0.6, 1e-9);

    // A family that does not fit is refused, with the numbers the menu needs
    // to suggest another camp.
    Family tooBig("Rupa Nath", "Laharighat");
    for (int i = 0; i < 5; i++) {
        tooBig.addMember(new Adult("Nath " + std::to_string(i), 30 + i, 'F', tooBig.getId(), 0));
    }
    ASSERT_THROWS(camp.admit(tooBig), OverCapacityException);
    ASSERT_EQ(camp.getOccupancy(), 6);          // the refused admission changed nothing
    bool caught = false;
    try {
        camp.admit(tooBig);
    } catch (const OverCapacityException& e) {
        caught = true;
        ASSERT_EQ(e.getCampId(), camp.getId());
        ASSERT_EQ(e.getRequested(), 5);
        ASSERT_EQ(e.getFreeSpace(), 4);
    }
    ASSERT_TRUE(caught);

    // An empty family is a no-op, not an error.
    Family empty("Nobody", "Nowhere");
    camp.admit(empty);
    ASSERT_EQ(camp.getOccupancy(), 6);

    // Boundary: exactly filling the camp is allowed, one more is not.
    Family fourMore("Exact Fit", "Bhuragaon");
    for (int i = 0; i < 4; i++) {
        fourMore.addMember(new Adult("Fit " + std::to_string(i), 20 + i, 'M', fourMore.getId(), 0));
    }
    camp.admit(fourMore);
    ASSERT_EQ(camp.getOccupancy(), 10);
    ASSERT_EQ(camp.freeSpace(), 0);
    ASSERT_TRUE(camp.isOverflowing());
    ASSERT_THROWS(++camp, OverCapacityException);
    ASSERT_EQ(camp.getOccupancy(), 10);
    camp.release(fourMore);
    ASSERT_EQ(camp.getOccupancy(), 6);
    ASSERT_EQ(camp.freeSpace(), 4);
    ASSERT_FALSE(camp.isOverflowing());

    // Shelters keep their own count; assigning people to one does not move the camp's.
    camp.shelterAt(0).occupy(6);
    ASSERT_EQ(camp.shelterAt(0).freeSpace(), 0);
    ASSERT_EQ(camp.getOccupancy(), 6);
    camp.shelterAt(0).vacate(6);

    // release() gives the space back.
    camp.release(big);
    ASSERT_EQ(camp.getOccupancy(), 0);
    camp.release(big);                          // clamps at zero
    ASSERT_EQ(camp.getOccupancy(), 0);

    // --- one more person, one fewer person ---
    Camp small("Tent Cluster", "Morigaon", 3);
    ++small;                                    // prefix
    ASSERT_EQ(small.getOccupancy(), 1);
    ++small;
    ASSERT_EQ(small.getOccupancy(), 2);

    Camp before = small++;                      // postfix returns the OLD value
    ASSERT_EQ(before.getOccupancy(), 2);
    ASSERT_EQ(small.getOccupancy(), 3);

    // The capacity guard applies to the operator too.
    ASSERT_THROWS(++small, OverCapacityException);
    ASSERT_EQ(small.getOccupancy(), 3);
    ASSERT_TRUE(small.isOverflowing());         // full counts as overflowing
    ASSERT_NEAR(small.occupancyRate(), 1.0, 1e-9);

    --small;                                    // prefix
    ASSERT_EQ(small.getOccupancy(), 2);
    Camp beforeDec = small--;                   // postfix
    ASSERT_EQ(beforeDec.getOccupancy(), 2);
    ASSERT_EQ(small.getOccupancy(), 1);
    --small;
    --small;                                    // clamps at zero
    ASSERT_EQ(small.getOccupancy(), 0);

    // --- the camp owns its stock ---
    camp.stockRef().set(Item::Rice, 400.0);
    ASSERT_NEAR(camp.getStock().get(Item::Rice), 400.0, 1e-9);
    camp.stockRef().setDailyDemand(Ration(100));
    ASSERT_NEAR(camp.getStock().daysOfSupplyFor(Item::Rice), 10.0, 1e-9);

    // --- copy constructor ---
    Camp copiedCamp(camp);
    ASSERT_STR_EQ(copiedCamp.getName(), "Bhuragaon HS School");
    ASSERT_EQ(copiedCamp.shelterCount(), 2);
    ASSERT_NEAR(copiedCamp.getStock().get(Item::Rice), 400.0, 1e-9);
    copiedCamp.stockRef().set(Item::Rice, 1.0);
    ASSERT_NEAR(camp.getStock().get(Item::Rice), 400.0, 1e-9);   // independent

    // --- Registry<Camp>: the same template as Registry<Family> ---
    Registry<Camp> camps;
    Camp* c1 = camps.add(std::unique_ptr<Camp>(new Camp("School", "Morigaon", 50)));
    Camp* c2 = camps.add(std::unique_ptr<Camp>(new Camp("Hall", "Morigaon", 30)));
    ASSERT_EQ(camps.size(), 2);
    ASSERT_TRUE(camps.findById(c2->getId()) == c2);
    ASSERT_STR_EQ(c1->getName(), "School");

    // setOccupancy and adoptId support CSV loading.
    c1->setOccupancy(17);
    ASSERT_EQ(c1->getOccupancy(), 17);
    ASSERT_THROWS(c1->setOccupancy(-1), std::invalid_argument);
    ASSERT_EQ(c1->getOccupancy(), 17);
    ASSERT_THROWS(c1->setOccupancy(51), OverCapacityException);
    ASSERT_EQ(c1->getOccupancy(), 17);
    bool setCaught = false;
    try {
        c1->setOccupancy(51);
    } catch (const OverCapacityException& e) {
        setCaught = true;
        ASSERT_EQ(e.getFreeSpace(), 50);
    }
    ASSERT_TRUE(setCaught);
    c1->adoptId(900);
    ASSERT_EQ(c1->getId(), 900);

    std::ostringstream oss;
    oss << camp;
    ASSERT_TRUE(oss.str().find("Bhuragaon HS School") != std::string::npos);
    ASSERT_TRUE(oss.str().find("Morigaon") != std::string::npos);

    std::ostringstream shelterOss;
    shelterOss << camp.shelterAt(0);
    ASSERT_TRUE(shelterOss.str().find("Room-1") != std::string::npos);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    // --- findShelterFor: first shelter that holds the whole group ---
    Camp placing("Placing", "Test", 20);
    ASSERT_EQ(placing.findShelterFor(1), -1);               // no shelters at all
    placing.addShelter(Camp::Shelter("S1", 6));
    placing.addShelter(Camp::Shelter("S2", 4));
    ASSERT_EQ(placing.findShelterFor(6), 0);                // exact fit
    ASSERT_EQ(placing.findShelterFor(3), 0);                // first fit wins
    placing.shelterAt(0).occupy(3);                         // S1 has 3 left, S2 has 4
    ASSERT_EQ(placing.findShelterFor(4), 1);                // skips the full-ish one
    ASSERT_EQ(placing.findShelterFor(5), -1);               // camp has 7 free, no single shelter
    ASSERT_EQ(placing.findShelterFor(7), -1);               // larger than every shelter
}
