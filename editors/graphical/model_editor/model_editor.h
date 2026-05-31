#ifndef MODEL_EDITOR_H
#define MODEL_EDITOR_H

#include <QHBoxLayout>
#include <QVulkanInstance>
#include <QWidget>

#include "model_left_pane.h"
#include "../vulkan/vulkan_window.h"
#include "../../../geometry/mesh_data.h"

class ModelEditor : public QWidget {
    Q_OBJECT

public:
    explicit ModelEditor(std::shared_ptr<MeshData> meshData, const QString& fullPath, int targetId,
                         QVulkanInstance* instance, bool isBinary, QWidget* parent = nullptr);
    std::vector<std::pair<std::string, std::string>> getPatchChanges();
    void updateModel(std::shared_ptr<MeshData> newMesh);
    bool isSurfacePatched() { return m_isSurfaceChanged; };
    void setSurfaceChanged(bool val) { m_isSurfaceChanged = val; };

signals:
    void surfacePatchRequested(double featureAngle, const QString&, int, bool);
    void surfaceCheckRequested(const QString&, int, bool);
    void dirtyStateChanged(bool isDirty);

private:
    bool m_isBinary, m_isSurfaceChanged = false;
    int m_targetId;
    QString m_fullPath;
    std::vector<std::string> m_patchNames;

    ModelLeftPane* m_leftPane;
    VulkanWindow* m_vulkanWindow;
    QVulkanInstance* m_vulkanInstance;
    std::shared_ptr<MeshData> m_meshData;

private slots:
    void onSurfacePatchRequest(double featureAngle);
    void onSurfaceCheckRequest();
};

#endif // MODEL_EDITOR_H