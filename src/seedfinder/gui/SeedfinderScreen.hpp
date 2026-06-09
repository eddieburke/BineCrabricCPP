#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "seedfinder/gui/BiomeMapWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "seedfinder/config/ConfigSchema.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace net::minecraft::client::gui::widget {
class SeedSpecEditor;
}

namespace net::minecraft::client::gui::screen::world {

class SeedfinderScreen : public screen::Screen {
public:
    SeedfinderScreen(screen::ScreenFactory parentFactory,
        std::string returnWorldName,
        std::string returnSeedText);

    ~SeedfinderScreen() override;

    void init() override;
    void tick() override;
    void removed() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void keyPressed(char character, int keyCode) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

protected:
    void buttonClicked(widget::ButtonWidget& button) override;

private:
    struct SeedResultRow {
        std::uint64_t seed = 0;
        float score = 0.f;
        int spawnX = 0;
        int spawnY = 0;
        int spawnZ = 0;
        std::uint8_t biomeId = 0;
        bool allHard = false;
    };

    // A labeled numeric input on the main form. Label is always drawn directly
    // above its field, so the two can never drift out of alignment.
    struct FormField {
        std::unique_ptr<widget::TextFieldWidget>* slot = nullptr;
        const char* label = "";
        bool allowSign = false;
        bool allowDot = false;
        int x = 0;
        int y = 0;
        int w = 0;
    };

    class ResultListWidget;

    [[nodiscard]] seedfinder::config::SearchConfig buildSearchConfig() const;
    [[nodiscard]] float minScoreThreshold() const;

    struct FieldTexts {
        std::string rangeStart;
        std::string rangeEnd;
        std::string scanRadius;
        std::string minScore;
        std::string threads;
        std::string topK;
        std::string uniqueBiomeMin;
        std::string minBiomeCoverage;
        std::string minBiomeRadius;
        std::string minBiomeCompactness;
    };

    [[nodiscard]] FieldTexts snapshotFieldTexts() const;
    void layoutForm(const FieldTexts* restore = nullptr);
    void layoutResultList();
    [[nodiscard]] bool fieldValid(const FormField& def) const;
    [[nodiscard]] bool charAllowedFor(const FormField& def, char character) const;

    void startSearch();
    void stopSearch();
    void returnToCreateWorld(const std::string& seedText);
    void mergePendingResults();
    void applySelectedSeed();
    void refreshBiomeMap();
    void cycleSpawnBiome(int dir);
    void updateSpawnBiomeButton();
    void updateSpecButton();
    void openSpecEditor();
    void closeSpecEditor(bool apply);
    void forEachField(const std::function<void(widget::TextFieldWidget&)>& fn);
    void forEachField(const std::function<void(const widget::TextFieldWidget&)>& fn) const;

    screen::ScreenFactory parentFactory_;
    std::string returnWorldName_;
    std::string returnSeedText_;

    std::unique_ptr<widget::TextFieldWidget> rangeStartField_;
    std::unique_ptr<widget::TextFieldWidget> rangeEndField_;
    std::unique_ptr<widget::TextFieldWidget> scanRadiusField_;
    std::unique_ptr<widget::TextFieldWidget> minScoreField_;
    std::unique_ptr<widget::TextFieldWidget> threadsField_;
    std::unique_ptr<widget::TextFieldWidget> topKField_;
    std::unique_ptr<widget::TextFieldWidget> uniqueBiomeMinField_;
    std::unique_ptr<widget::TextFieldWidget> minBiomeCoverageField_;
    std::unique_ptr<widget::TextFieldWidget> minBiomeRadiusField_;
    std::unique_ptr<widget::TextFieldWidget> minBiomeCompactnessField_;
    std::vector<FormField> formFields_;

    std::unique_ptr<ResultListWidget> resultList_;
    std::unique_ptr<widget::BiomeMapWidget> biomeMap_;
    std::unique_ptr<widget::SeedSpecEditor> specEditor_;
    std::vector<seedfinder::config::RuleNode> specNodes_;
    bool specEditorOpen_ = false;

    std::uint64_t mapPreviewSeed_ = 0;
    int mapPreviewRadius_ = 0;
    int spawnBiomeIndex_ = 0;
    int formBottom_ = 0;
    int resultTop_ = 0;
    int panelX_ = 0;
    int panelW_ = 0;
    bool firstInit_ = true;
    widget::ActionButtonWidget* searchButton_ = nullptr;
    widget::ActionButtonWidget* stopButton_ = nullptr;
    widget::ActionButtonWidget* useSeedButton_ = nullptr;
    widget::ActionButtonWidget* depthButton_ = nullptr;
    widget::ActionButtonWidget* spawnBiomeButton_ = nullptr;
    widget::ActionButtonWidget* specButton_ = nullptr;

    std::uint8_t probeDepth_ = 1;
    int selectedResultIndex_ = -1;
    std::vector<SeedResultRow> results_;

    std::atomic<bool> cancelFlag_ {false};
    std::atomic<bool> searchRunning_ {false};
    std::atomic<bool> searchFinished_ {false};
    std::atomic<std::uint64_t> seedsChecked_ {0};
    std::mutex resultsMutex_;
    std::vector<SeedResultRow> pendingResults_;
    std::thread searchThread_;
};

} // namespace net::minecraft::client::gui::screen::world
