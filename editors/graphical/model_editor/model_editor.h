#ifndef MODEL_EDITOR_H
#define MODEL_EDITOR_H

#include <QHBoxLayout>
#include <QVulkanInstance>
#include <QWidget>

#include "model_left_pane.h"
#include "../vulkan/vulkan_window.h"
#include "../../../geometry/graphic_data.h"

class ModelEditor : public QWidget {
    Q_OBJECT

public:
    explicit ModelEditor(std::shared_ptr<RenderData> renderData, const QString& fullPath, int targetId,
                         QVulkanInstance* instance, bool isBinary, QWidget* parent = nullptr);
    std::vector<std::pair<std::string, std::string>> getPatchChanges();
    void updateModel(std::shared_ptr<RenderData> newMesh);
    bool isSurfacePatched() { return m_isSurfaceChanged; };
    void setSurfaceChanged(bool val) { m_isSurfaceChanged = val; };
    void changeBounds(double scaleFactor);

signals:
    void surfaceCheckRequested(const QString& fullPath, int targetId, bool isBinary);
    void surfacePatchRequested(double featureAngle, const QString& fullPath, int targetId, bool isBinary);
    void surfaceScaleRequested(double scaleFactor, const QString& fullPath, int targetId);
    void dirtyStateChanged(bool isDirty);

private:
    bool m_isBinary, m_isSurfaceChanged = false;
    int m_targetId;
    QString m_fullPath;
    std::vector<std::string> m_patchNames;

    ModelLeftPane* m_leftPane;
    VulkanWindow* m_vulkanWindow;
    QVulkanInstance* m_vulkanInstance;
    std::shared_ptr<RenderData> m_renderData;

private slots:
    void onSurfaceCheckRequest();
    void onSurfacePatchRequest(double featureAngle);
    void onSurfaceScaleRequest(double scaleFactor);
};

#endif // MODEL_EDITOR_H