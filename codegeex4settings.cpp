// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "codegeex4settings.h"

#include "codegeex4constants.h"
#include "codegeex4tr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/project.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QToolTip>

using namespace Utils;

namespace CodeGeeX4 {

static void initEnableAspect(BoolAspect &enableCodeGeeX4)
{
    enableCodeGeeX4.setSettingsKey(Constants::ENABLE_CODEGEEX4);
    enableCodeGeeX4.setDisplayName(Tr::tr("Enable CodeGeeX4"));
    enableCodeGeeX4.setLabelText(Tr::tr("Enable CodeGeeX4"));
    enableCodeGeeX4.setToolTip(Tr::tr("Enables the CodeGeeX4 integration."));
    enableCodeGeeX4.setDefaultValue(false);
}

CodeGeeX4Settings &settings()
{
    static CodeGeeX4Settings settings;
    return settings;
}

CodeGeeX4Settings::CodeGeeX4Settings()
{
    setAutoApply(false);

    autoComplete.setDisplayName(Tr::tr("Auto Complete"));
    autoComplete.setSettingsKey("CodeGeeX4.Autocomplete");
    autoComplete.setLabelText(Tr::tr("Request completions automatically"));
    autoComplete.setDefaultValue(true);
    autoComplete.setEnabler(&enableCodeGeeX4);
    autoComplete.setToolTip(Tr::tr("Automatically request suggestions for the current text cursor "
                                   "position after changes to the document."));

    url.setDefaultValue("http://127.0.0.1:1911/v1/completions");
    url.setDisplayStyle(Utils::StringAspect::LineEditDisplay);
    url.setSettingsKey("CodeGeeX4.URL");
    url.setLabelText(Tr::tr("URL of CodeGeeX4 API:"));
    url.setHistoryCompleter("CodeGeeX4.URL.History");
    url.setDisplayName(Tr::tr("CodeGeeX4 API URL"));
    url.setEnabler(&enableCodeGeeX4);
    url.setToolTip(Tr::tr("Input URL of CodeGeeX4 API."));

    contextLimit.setDefaultValue(16*1000);
    contextLimit.setRange(100,64*1000);
    contextLimit.setSettingsKey("CodeGeeX4.ContextLimit");
    contextLimit.setLabelText(Tr::tr("Context length limit:"));
    contextLimit.setDisplayName(Tr::tr("Context length limit"));
    contextLimit.setEnabler(&enableCodeGeeX4);
    contextLimit.setToolTip(Tr::tr("Maximum length of context send to server."));

    length.setDefaultValue(20);
    length.setRange(1,8*1000);
    length.setSettingsKey("CodeGeeX4.Length");
    length.setLabelText(Tr::tr("Output sequence length:"));
    length.setDisplayName(Tr::tr("Output sequence length"));
    length.setEnabler(&enableCodeGeeX4);
    length.setToolTip(Tr::tr("Number of tokens to generate each time."));

    // temperarure.setDefaultValue(0.2);
    // temperarure.setRange(0.0,1.0);
    // temperarure.setSettingsKey("CodeGeeX4.Temperarure");
    // temperarure.setLabelText(Tr::tr("Temperature:"));
    // temperarure.setDisplayName(Tr::tr("Temperature"));
    // temperarure.setEnabler(&enableCodeGeeX4);
    // temperarure.setToolTip(Tr::tr("Affects how \"random\" the model’s output is."));

    // topK.setDefaultValue(0);
    // topK.setRange(0,100);
    // topK.setSettingsKey("CodeGeeX4.TopK");
    // topK.setLabelText(Tr::tr("Top K:"));
    // topK.setDisplayName(Tr::tr("Top K"));
    // topK.setEnabler(&enableCodeGeeX4);
    // topK.setToolTip(Tr::tr("Affects how \"random\" the model's output is."));

    // topP.setDefaultValue(0.95);
    // topP.setRange(0.0,1.0);
    // topP.setSettingsKey("CodeGeeX4.TopP");
    // topP.setLabelText(Tr::tr("Top P:"));
    // topP.setDisplayName(Tr::tr("Top P"));
    // topP.setEnabler(&enableCodeGeeX4);
    // topP.setToolTip(Tr::tr("Affects how \"random\" the model's output is."));

    expandHeaders.setDisplayName(Tr::tr("Try to expand headers (experimnetal)"));
    expandHeaders.setSettingsKey("CodeGeeX2.ExpandHeaders");
    expandHeaders.setLabelText(Tr::tr("Try to expand headers (experimnetal)"));
    expandHeaders.setDefaultValue(true);
    expandHeaders.setEnabler(&enableCodeGeeX4);
    expandHeaders.setToolTip(Tr::tr("Try to expand headers when sending requests."));

    initEnableAspect(enableCodeGeeX4);

    readSettings();

    setLayouter([this] {
        using namespace Layouting;

        return Column {
            enableCodeGeeX4, br,
            autoComplete, br,
            url, br,
            contextLimit, br,
            length, br,
            // temperarure, br,
            // topK, br,
            // topP, br,
            expandHeaders, br,
            st
        };
    });
}

CodeGeeX4ProjectSettings::CodeGeeX4ProjectSettings(ProjectExplorer::Project *project)
{
    setAutoApply(true);

    useGlobalSettings.setSettingsKey(Constants::CODEGEEX4_USE_GLOBAL_SETTINGS);
    useGlobalSettings.setDefaultValue(true);

    initEnableAspect(enableCodeGeeX4);

    Store map = storeFromVariant(project->namedSettings(Constants::CODEGEEX4_PROJECT_SETTINGS_ID));
    fromMap(map);

    connect(&enableCodeGeeX4, &BaseAspect::changed, this, [this, project] { save(project); });
    connect(&useGlobalSettings, &BaseAspect::changed, this, [this, project] { save(project); });
}

void CodeGeeX4ProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    useGlobalSettings.setValue(useGlobal);
}

bool CodeGeeX4ProjectSettings::isEnabled() const
{
    if (useGlobalSettings())
        return settings().enableCodeGeeX4();
    return enableCodeGeeX4();
}

void CodeGeeX4ProjectSettings::save(ProjectExplorer::Project *project)
{
    Store map;
    toMap(map);
    project->setNamedSettings(Constants::CODEGEEX4_PROJECT_SETTINGS_ID, variantFromStore(map));

    // This triggers a restart of the CodeGeeX4 language server.
    settings().apply();
}

class CodeGeeX4SettingsPage : public Core::IOptionsPage
{
public:
    CodeGeeX4SettingsPage()
    {
        setId(Constants::CODEGEEX4_GENERAL_OPTIONS_ID);
        setDisplayName("CodeGeeX4");
        setCategory(Constants::CODEGEEX4_GENERAL_OPTIONS_CATEGORY);
        setDisplayCategory(Constants::CODEGEEX4_GENERAL_OPTIONS_DISPLAY_CATEGORY);
        setCategoryIconPath(":/codegeex4/images/settingscategory_codegeex2.png");
        setSettingsProvider([] { return &settings(); });
    }
};

const CodeGeeX4SettingsPage settingsPage;

} // namespace CodeGeeX4
