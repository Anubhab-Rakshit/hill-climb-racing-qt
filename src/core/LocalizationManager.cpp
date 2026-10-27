#include "LocalizationManager.h"
#include <algorithm>

namespace Core {

LocalizationManager& LocalizationManager::instance() {
    static LocalizationManager s_instance;
    return s_instance;
}

LocalizationManager::LocalizationManager()
    : m_language(Language::ENGLISH)
{
}

std::string LocalizationManager::getStageName(const std::string& stageId) const {
    if (m_language == Language::ENGLISH) {
        if (stageId == "countryside") return "COUNTRYSIDE";
        if (stageId == "desert") return "DESERT";
        if (stageId == "arctic") return "ARCTIC";
        if (stageId == "highway" || stageId == "neon") return "NEON HIGHWAY";
        if (stageId == "moon") return "MOON";
        if (stageId == "mountain") return "MOUNTAIN";
        if (stageId == "volcano") return "VOLCANO";
        if (stageId == "super_countryside") return "SUPER COUNTRYSIDE";
        if (stageId == "seasons") return "CHANGING SEASONS";
        return stageId;
    }

    // Bengali Mode (বাংলা মোড)
    if (stageId == "countryside") return "GOVINDAPUR";
    if (stageId == "desert") return "THAR MARUBHUMI";
    if (stageId == "arctic") return "TUNDRAPUR (BARAF)";
    if (stageId == "highway" || stageId == "neon") return "JHIKIMIKI";
    if (stageId == "moon") return "CHANDRAJAAN";
    if (stageId == "mountain") return "DARJEELING";
    if (stageId == "volcano") return "AGNIGIRI";
    if (stageId == "super_countryside") return "BOROBABU GOVINDAPUR";
    if (stageId == "seasons") return "RITURONGO";
    return stageId;
}

std::string LocalizationManager::getStageTagline(const std::string& stageId) const {
    if (m_language == Language::ENGLISH) {
        if (stageId == "countryside") return "Rolling emerald hills & pastoral tranquility";
        if (stageId == "desert") return "Arid dunes with shifting sands & loose traction";
        if (stageId == "arctic") return "Sub-zero ice sheets with minimal tyre grip";
        if (stageId == "highway" || stageId == "neon") return "Synthetic cyber grid with neon light corridors";
        if (stageId == "moon") return "Extreme low gravity with mammoth crater jumps";
        if (stageId == "mountain") return "Dangerous 45-degree rock faces & cliff drops";
        if (stageId == "volcano") return "Smoldering basalt peaks and geothermal fissures";
        if (stageId == "super_countryside") return "Extreme rollercoaster hills pushing suspension limits";
        if (stageId == "seasons") return "Spring thaw, summer heat, autumn fog, winter ice";
        return "Classic rally circuit";
    }

    // Bengali Mode
    if (stageId == "countryside") return "Kancha rasta aar dhaan khet - jhakuni khete khete cholo!";
    if (stageId == "desert") return "Balir choray tyre slip, sabdhane handle ghoran!";
    if (stageId == "arctic") return "Pichhle baraf er changor bhenge baraf er jhor!";
    if (stageId == "highway" || stageId == "neon") return "Cyber raat aar neon আলোy chokher polok felben na!";
    if (stageId == "moon") return "Kom madhyakarshone chander deshe lamba lamba laaf!";
    if (stageId == "mountain") return "Khara paharer banke banke kuasha aar chaa baagan!";
    if (stageId == "volcano") return "Lavar upor drive, ektu bhul holei gari bhasma!";
    if (stageId == "super_countryside") return "Roller-coaster er moton khara dhal, kolje shukiye jaabe!";
    if (stageId == "seasons") return "Grishmo, borsha, shorot, sheet - chaar ritur khellam!";
    return "Khapchhada circuit!";
}

std::string LocalizationManager::getVehicleName(int vehicleType) const {
    if (m_language == Language::ENGLISH) {
        switch (vehicleType) {
            case 0: return "HILL CLIMBER";
            case 1: return "MOTOCROSS BIKE";
            case 2: return "MONSTER TRUCK";
            case 3: return "QUAD BIKE";
            case 4: return "OFFROADER";
            case 5: return "RALLY CAR";
            case 6: return "TRACTOR";
            case 7: return "FORMULA RACER";
            case 8: return "PICKUP TRUCK";
            case 9: return "SUPERCAR";
            default: return "RACER RIG";
        }
    }

    // Bengali Mode
    switch (vehicleType) {
        case 0: return "CHOTPOTEY GAARI";
        case 1: return "PHATPHATI (BIKE)";
        case 2: return "GUNDA GARI";
        case 3: return "CHAAR CHAKAR GHODA";
        case 4: return "JONGOL GAARI";
        case 5: return "DHOOM MACHALE CAR";
        case 6: return "CHASHIR RATH";
        case 7: return "ROCKET CAR";
        case 8: return "MALBAHI GAARI";
        case 9: return "TOOFAN GAARI";
        default: return "DESHI GAARI";
    }
}

std::string LocalizationManager::getVehicleTagline(int vehicleType) const {
    if (m_language == Language::ENGLISH) {
        switch (vehicleType) {
            case 0: return "Classic reliable 4x4 offroad jeep";
            case 1: return "Ultra-light agile dirtbike with insane airtime rotation";
            case 2: return "Heavy behemoth with gargantuan balloon tyres";
            case 3: return "All-terrain 4WD quad bike with balanced drift stability";
            case 4: return "Rugged long-wheelbase expedition utility rig";
            case 5: return "High-RPM turbo rally special with sharp downforce";
            case 6: return "Heavy iron beast with gargantuan rear paddle traction";
            case 7: return "Ground-effect aero missile built for extreme velocity";
            case 8: return "Workhorse diesel truck with weighted leaf suspension";
            case 9: return "V12 twin-turbo hypercar with carbon aerodynamics";
            default: return "Custom racing machine";
        }
    }

    // Bengali Mode
    switch (vehicleType) {
        case 0: return "Sadharon kintu bhorosha joggo 4x4 jeep";
        case 1: return "Halka phatphati, haway ultano khub shohoj!";
        case 2: return "Bishaal chakaar gunda gari, samne ja asbe guro hobe!";
        case 3: return "Chaar chakar ghoda, sob rastay daurabe beporowa!";
        case 4: return "Jongol pahar paar korar shokto rig";
        case 5: return "Dhoom machano speed aar daabani!";
        case 6: return "Gramer math er kankra chaka rath!";
        case 7: return "Rocket er moto chhutbe pista dhalay!";
        case 8: return "Bojhai gari, dheere kintu pother raja!";
        case 9: return "Jhorer moto toofan gari!";
        default: return "Ajob gari!";
    }
}

std::string LocalizationManager::getStuntTitle(const std::string& stuntKey) const {
    if (m_language == Language::ENGLISH) return stuntKey;

    // Bengali Mode
    if (stuntKey.find("AIR TIME") != std::string::npos || stuntKey.find("Air Time") != std::string::npos) {
        return "HAWAY URA!";
    }
    if (stuntKey.find("BACKFLIP") != std::string::npos || stuntKey.find("Backflip") != std::string::npos) {
        return "ULTO POLTI (+1000)";
    }
    if (stuntKey.find("FRONTFLIP") != std::string::npos || stuntKey.find("Frontflip") != std::string::npos) {
        return "SAMNE POLTI (+1000)";
    }
    if (stuntKey.find("WHEELIE") != std::string::npos || stuntKey.find("Wheelie") != std::string::npos) {
        return "EK CHAKAY NACH!";
    }
    if (stuntKey.find("CHECKPOINT") != std::string::npos || stuntKey.find("Checkpoint") != std::string::npos) {
        return "PORBO PAAR!";
    }
    if (stuntKey.find("FINISH") != std::string::npos) {
        return "JAI HIND! FINISH!";
    }
    return stuntKey;
}

std::string LocalizationManager::getUIString(const std::string& key) const {
    if (m_language == Language::ENGLISH) {
        if (key == "GAS") return "GAS";
        if (key == "BRAKE") return "BRAKE";
        if (key == "GAME_OVER") return "GAME OVER";
        if (key == "OUT_OF_FUEL") return "OUT OF FUEL!";
        if (key == "DRIVER_DOWN") return "DRIVER DOWN (NECK SNAP)!";
        if (key == "VIP_EVALUATOR") return "VIP EVALUATOR MODE";
        if (key == "DEV_MODE") return "DEVELOPER CONSOLE";
        if (key == "GRAND_PRIX") return "GRAND PRIX CHALLENGE";
        if (key == "GARAGE") return "GARAGE & TUNING";
        if (key == "STAGE_SELECT") return "STAGE SELECT";
        if (key == "COINS") return "COINS";
        if (key == "SPEED") return "SPEED";
        if (key == "DISTANCE") return "DISTANCE";
        return key;
    }

    // Bengali Mode
    if (key == "GAS") return "CHALAO!";
    if (key == "BRAKE") return "THAMO!";
    if (key == "GAME_OVER") return "GARI DHOBA! KHELA SHESH";
    if (key == "OUT_OF_FUEL") return "TEL SHESH! GARI BONDHO";
    if (key == "DRIVER_DOWN") return "GHAR MOTKE GECHE! (CRASH)";
    if (key == "VIP_EVALUATOR") return "PROFESSOR BABUR PASS";
    if (key == "DEV_MODE") return "ENGINEER BABUR CONSOLE";
    if (key == "GRAND_PRIX") return "SERADER LORAI (CHAMPION)";
    if (key == "GARAGE") return "GARIR KARKHANA";
    if (key == "STAGE_SELECT") return "POHTH BACHHO";
    if (key == "COINS") return "POYSA";
    if (key == "SPEED") return "GATI";
    if (key == "DISTANCE") return "DUROTTO";
    return key;
}

} // namespace Core
