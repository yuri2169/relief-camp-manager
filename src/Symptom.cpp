#include "Symptom.h"
#include <stdexcept>

std::string symptomName(Symptom s) {
    switch (s) {
        case Symptom::Diarrhoea:     return "Diarrhoea";
        case Symptom::Fever:         return "Fever";
        case Symptom::SkinInfection: return "Skin infection";
        case Symptom::Respiratory:   return "Respiratory";
        case Symptom::Other:         return "Other";
    }
    return "Unknown";
}

Symptom symptomFromName(const std::string& name) {
    for (int i = 0; i < SYMPTOM_COUNT; i++) {
        Symptom s = static_cast<Symptom>(i);
        if (symptomName(s) == name) {
            return s;
        }
    }
    throw std::invalid_argument("unknown symptom: " + name);
}
