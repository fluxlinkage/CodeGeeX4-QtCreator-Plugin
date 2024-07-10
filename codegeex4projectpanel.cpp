// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegeex4projectpanel.h"

#include "codegeex4constants.h"
#include "codegeex4settings.h"
#include "codegeex4tr.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>

#include <utils/layoutbuilder.h>

using namespace ProjectExplorer;

namespace CodeGeeX4::Internal {

class CodeGeeX4ProjectSettingsWidget final : public ProjectSettingsWidget
{
public:
    CodeGeeX4ProjectSettingsWidget()
    {
        setGlobalSettingsId(Constants::CODEGEEX4_GENERAL_OPTIONS_ID);
        setUseGlobalSettingsCheckBoxVisible(true);
    }
};

static ProjectSettingsWidget *createCodeGeeX4ProjectPanel(Project *project)
{
    using namespace Layouting;

    auto widget = new CodeGeeX4ProjectSettingsWidget;
    auto settings = new CodeGeeX4ProjectSettings(project);
    settings->setParent(widget);

    QObject::connect(widget,
                     &ProjectSettingsWidget::useGlobalSettingsChanged,
                     settings,
                     &CodeGeeX4ProjectSettings::setUseGlobalSettings);

    widget->setUseGlobalSettings(settings->useGlobalSettings());
    widget->setEnabled(!settings->useGlobalSettings());

    QObject::connect(widget,
                     &ProjectSettingsWidget::useGlobalSettingsChanged,
                     widget,
                     [widget](bool useGlobal) { widget->setEnabled(!useGlobal); });

    // clang-format off
    Column {
        settings->enableCodeGeeX4,
    }.attachTo(widget);
    // clang-format on

    return widget;
}

class CodeGeeX4ProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CodeGeeX4ProjectPanelFactory()
    {
        setPriority(1000);
        setDisplayName(Tr::tr("CodeGeeX4"));
        setCreateWidgetFunction(&createCodeGeeX4ProjectPanel);
    }
};

void setupCodeGeeX4ProjectPanel()
{
    static CodeGeeX4ProjectPanelFactory theCodeGeeX4ProjectPanelFactory;
}

} // namespace CodeGeeX4::Internal
