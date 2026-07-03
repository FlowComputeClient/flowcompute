// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

#ifndef EDITORS_GRAPHICAL_RESULT_RESULT_EDITOR_H_
#define EDITORS_GRAPHICAL_RESULT_RESULT_EDITOR_H_

#include <QHBoxLayout>
#include <QVulkanInstance>
#include <QWidget>

#include "../vulkan/vulkan_window.h"
#include "../../../geometry/graphic_data.h"
#include "result_left_pane.h"

class MainWindow;

class ResultEditor : public QWidget {
    Q_OBJECT

 public:
    explicit ResultEditor(const QStringList& timeFolders,
                          std::shared_ptr<RenderData> renderData,
                          const QString& casePath, int targetId,
                          QVulkanInstance* instance, QWidget* parent = nullptr);
    void updateResult(std::shared_ptr<RenderData> newData);
    void applyTheme(const QString& theme);

 signals:
    void dirtyStateChanged(bool isDirty);
    void timeChanged(const QString& casePath, int targetId,
                     const QString& timeFolder);

 private:
    MainWindow* m_mainWin;
    ResultLeftPane* m_leftPane;
    bool m_isSurfaceChanged = false;
    int m_targetId;
    QString m_casePath;
    std::vector<std::string> m_patchNames;

    VulkanWindow* m_vulkanWindow;
    QVulkanInstance* m_vulkanInstance;
    std::shared_ptr<RenderData> m_renderData;

 private slots:
    void onTimeChange(const QString& time);
};

#endif  // EDITORS_GRAPHICAL_RESULT_RESULT_EDITOR_H_