#ifndef RELIEF_SYMPTOM_H
#define RELIEF_SYMPTOM_H

#include <string>

// The symptoms a health report can record.
enum class Symptom { Diarrhoea, Fever, SkinInfection, Respiratory, Other };

const int SYMPTOM_COUNT = 5;

std::string symptomName(Symptom s);
Symptom symptomFromName(const std::string& name);   // throws std::invalid_argument

#endif
