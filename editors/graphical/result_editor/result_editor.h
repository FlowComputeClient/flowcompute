#ifndef RESULT_EDITOR_H
#define RESULT_EDITOR_H

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

signals:
    void dirtyStateChanged(bool isDirty);
    void meshCheckRequested(const QString& fullPath, int targetId);
    void meshPatchRequested(double featureAngle, const QString& fullPath, int targetId);
    void meshRenumberRequested(const QString& fullPath, int targetId);

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
};

#endif // RESULT_EDITOR_H