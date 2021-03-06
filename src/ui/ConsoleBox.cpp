#include <imgui/imgui.h>
#include <render/RenderSystem.h>
#include <utils/logger.h>
#include "ConsoleBox.h"
#include <engine/BaseEngine.h>
#include "zFont.h"
#include <ui/Console.h>

UI::ConsoleBox::ConsoleBox(Engine::BaseEngine& e, UI::Console& console) :
    View(e),
    m_Console(console)
{
    m_Config.height = 10;
    m_BackgroundTexture = e.getEngineTextureAlloc().loadTextureVDF("CONSOLE.TGA");
    m_SuggestionsBackgroundTexture = e.getEngineTextureAlloc().loadTextureVDF("DLG_CHOICE.TGA");
    m_CurrentlySelected = 0;
}

UI::ConsoleBox::~ConsoleBox()
{
}

void UI::ConsoleBox::update(double dt, Engine::Input::MouseState& mstate, Render::RenderConfig& config)
{
    m_IsHidden = !m_Console.isOpen();
    if (m_IsHidden)
        return;

    auto font = DEFAULT_FONT;
    auto fontSelected = DEFAULT_FONT_HI;
    const UI::zFont* fnt = m_Engine.getFontCache().getFont(font);
    if(!fnt)
        return;

    View::update(dt, mstate, config);

    // 13 pixel extra space to left screen edge
    const int xDistanceToEdge = 13;
    int suggestionXBorderSize;
    {
        int spaceWidth, dummyHeight;
        fnt->calcTextMetrics(" ", spaceWidth, dummyHeight);
        suggestionXBorderSize = spaceWidth;
    }
    unsigned int previewBefore = 9;
    unsigned int previewAfter = 9;
    // space between columns in pixel
    const int spaceBetweenColumns = 40;
    int consoleSizeX = config.state.viewWidth;
    int consoleSizeY = Math::iround(config.state.viewHeight * 0.25f);
    // -1, because we need space for the typed line
    const std::size_t maxNumConsoleHistoryLines = consoleSizeY / fnt->getFontHeight() - 1;

    // Draw console
    {
        // Draw console background image
        Textures::Texture& background = m_Engine.getEngineTextureAlloc().getTexture(m_BackgroundTexture);

        bgfx::ProgramHandle program = config.programs.imageProgram;
        drawTexture(BGFX_VIEW,
                    0, 0,
                    consoleSizeX, consoleSizeY,
                    config.state.viewWidth, config.state.viewHeight, background.m_TextureHandle, program,
                    config.uniforms.diffuseTexture);

        // draw console output + current line
        std::stringstream ss;
        auto& outputList = m_Console.getOutputLines();
        std::size_t numLines = std::min(outputList.size(), maxNumConsoleHistoryLines);
        std::vector<std::string> outputLines(numLines);
        std::copy_n(outputList.begin(), numLines, outputLines.rbegin());
        outputLines.push_back(m_Console.getTypedLine());
        auto joined = Utils::join(outputLines.begin(), outputLines.end(), "\n");
        drawText(joined, xDistanceToEdge, consoleSizeY, A_BottomLeft, config, font);
    }
    // Draw suggestions
    {
        const auto& suggestionsList = m_Console.getSuggestions();
        // check if suggestions for last token are empty
        if (suggestionsList.empty() || suggestionsList.back().empty())
            return;

        const auto& suggestions = suggestionsList.back();

        // find preview range
        m_CurrentlySelected = Math::clamp(m_CurrentlySelected, 0, static_cast<int>(suggestions.size() - 1));
        int shownStart = m_CurrentlySelected - previewBefore;
        // past the end index
        int shownEnd = m_CurrentlySelected + previewAfter + 1;
        if (shownStart < 0)
        {
            shownEnd += -shownStart;
            shownStart = 0;
        }
        if (shownEnd > static_cast<int>(suggestions.size()))
        {
            shownStart -= shownEnd - suggestions.size();
            shownEnd = suggestions.size();
        }
        // clamp start index
        shownStart = std::max(shownStart, 0);
        int currentlySelectedRelative = m_CurrentlySelected - shownStart;

        // fill columns
        std::size_t columnID = 0;
        std::vector<std::vector<std::string>> columns;
        std::vector<int> columnWidths;
        while (true)
        {
            bool columnEmpty = true;
            std::vector<std::string> column;
            int columnWidth = 0;
            for (int row = shownStart; row < shownEnd; row++)
            {
                auto& aliasList = suggestions.at(row)->aliasList;
                if (columnID < aliasList.size())
                {
                    columnEmpty = false;
                    int w, h;
                    fnt->calcTextMetrics(aliasList[columnID], w, h);
                    columnWidth = std::max(columnWidth, w);
                    column.push_back(aliasList[columnID]);
                } else {
                    column.push_back("");
                }
            }

            columnID++;
            if (columnEmpty)
                break;
            else
            {
                columns.push_back(column);
                columnWidths.push_back(columnWidth);
            }
        }
        int fontHeight = fnt->getFontHeight();
        int suggestionBoxSizeX = std::accumulate(columnWidths.begin(), columnWidths.end(), 0);
        suggestionBoxSizeX += (columns.size() - 1) * spaceBetweenColumns;
        int suggestionBoxSizeY = fontHeight * columns.front().size();
        // Get background image
        Textures::Texture& background = m_Engine.getEngineTextureAlloc().getTexture(m_SuggestionsBackgroundTexture);

        // find start of token which is currently auto-completed
        std::string strBefore = m_Console.getTypedLine();
        if (!strBefore.empty() && !isspace(strBefore.back()))
        {
            auto lastSpaceIt = std::find(strBefore.rbegin(), strBefore.rend(), ' ');
            if (lastSpaceIt == strBefore.rend())
                strBefore.clear();
            else{
                auto numNonSpacesAtEnd = lastSpaceIt - strBefore.rbegin();
                strBefore.erase(strBefore.end() - numNonSpacesAtEnd, strBefore.end());
            }
        }
        int strBeforeWidth;
        {
            int w, h;
            fnt->calcTextMetrics(strBefore, w, h);
            strBeforeWidth = w;
        }
        bgfx::ProgramHandle program = config.programs.imageProgram;
        drawTexture(BGFX_VIEW,
                    xDistanceToEdge + strBeforeWidth - suggestionXBorderSize, consoleSizeY,
                    suggestionBoxSizeX + 2 * suggestionXBorderSize, suggestionBoxSizeY,
                    config.state.viewWidth, config.state.viewHeight, background.m_TextureHandle, program,
                    config.uniforms.diffuseTexture);

        int colID = 0;
        int columnOffsetX = strBeforeWidth;
        for (auto& column : columns){
            std::vector<std::string> columnsSelected(column.size());
            std::swap(column.at(currentlySelectedRelative), columnsSelected.at(currentlySelectedRelative));
            auto joined = Utils::join(column.begin(), column.end(), "\n");
            drawText(joined, xDistanceToEdge + columnOffsetX, consoleSizeY + suggestionBoxSizeY, A_BottomLeft, config, font);
            auto joinedSelected = Utils::join(columnsSelected.begin(), columnsSelected.end(), "\n");
            drawText(joinedSelected, xDistanceToEdge + columnOffsetX, consoleSizeY + suggestionBoxSizeY, A_BottomLeft, config, fontSelected);
            columnOffsetX += columnWidths[colID++] + spaceBetweenColumns;
        }
    }
}

void UI::ConsoleBox::increaseSelectionIndex(int amount) {
    const auto& suggestionsList = m_Console.getSuggestions();
    // check if suggestions for last token are empty
    if (suggestionsList.empty() || suggestionsList.back().empty())
        return;

    int suggestionCount = static_cast<int>(suggestionsList.back().size());
    m_CurrentlySelected = Utils::mod(m_CurrentlySelected + amount, suggestionCount);
}

void UI::ConsoleBox::setSelectionIndex(int newIndex) {
    m_CurrentlySelected = newIndex;
}
