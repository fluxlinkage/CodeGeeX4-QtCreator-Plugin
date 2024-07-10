// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer { class Project; }

namespace CodeGeeX4 {

class CodeGeeX4Settings : public Utils::AspectContainer
{
public:
    CodeGeeX4Settings();

    Utils::BoolAspect enableCodeGeeX4{this};
    Utils::BoolAspect autoComplete{this};
    Utils::StringAspect url{this};
    Utils::IntegerAspect contextLimit{this};
    Utils::IntegerAspect length{this};
    //Utils::DoubleAspect temperarure{this};
    //Utils::IntegerAspect topK{this};
    //Utils::DoubleAspect topP{this};
    Utils::BoolAspect expandHeaders{this};
};

CodeGeeX4Settings &settings();

class CodeGeeX4ProjectSettings : public Utils::AspectContainer
{
public:
    explicit CodeGeeX4ProjectSettings(ProjectExplorer::Project *project);

    void save(ProjectExplorer::Project *project);
    void setUseGlobalSettings(bool useGlobalSettings);

    bool isEnabled() const;

    Utils::BoolAspect enableCodeGeeX4{this};
    Utils::BoolAspect useGlobalSettings{this};
};

} // namespace CodeGeeX4
