#pragma once

#include <string>
#include <unordered_map>

namespace Core {

enum class Language {
    ENGLISH = 0,
    BENGALI = 1
};

/**
 * @brief Centralized localization manager providing dual English and Bengali
 * ("বাংলা মোড") strings, including humorous colloquial Bengali translations.
 */
class LocalizationManager {
public:
    static LocalizationManager& instance();

    Language currentLanguage() const { return m_language; }
    void setLanguage(Language lang) { m_language = lang; }
    void toggleLanguage() {
        m_language = (m_language == Language::ENGLISH) ? Language::BENGALI : Language::ENGLISH;
    }
    bool isBengali() const { return m_language == Language::BENGALI; }

    // Translation queries
    std::string getStageName(const std::string& stageId) const;
    std::string getStageTagline(const std::string& stageId) const;
    std::string getVehicleName(int vehicleType) const;
    std::string getVehicleTagline(int vehicleType) const;
    std::string getStuntTitle(const std::string& stuntKey) const;
    std::string getUIString(const std::string& key) const;

private:
    LocalizationManager();
    Language m_language = Language::ENGLISH;
};

} // namespace Core
