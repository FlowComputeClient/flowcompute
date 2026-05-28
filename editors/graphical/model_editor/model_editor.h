#ifndef MODEL_EDITOR_H
#define MODEL_EDITOR_H

#include <QHBoxLayout>
#include <QSplitter>
#include <QVulkanInstance>
#include <QWidget>

#include "left_pane.h"
#include "../vulkan/vulkan_window.h"
#include "../../../geometry/mesh_data.h"

class QSplitter;
class QTextEdit;
class QListWidget;

class ModelEditor : public QWidget {
    Q_OBJECT

public:
    explicit ModelEditor(std::shared_ptr<MeshData> meshData, const QString& fullPath, int targetId,
                         QVulkanInstance* instance, bool isBinary, QWidget* parent = nullptr);

signals:
    void surfacePatchRequested(double featureAngle, const QString&, int, bool);
    void surfaceCheckRequested(const QString&, int, bool);

private:
    bool m_isBinary;
    int m_targetId;
    QString m_fullPath;
    QSplitter* m_splitter;
    LeftPane* m_leftPane;
    QVulkanInstance* m_vulkanInstance;
    std::shared_ptr<MeshData> m_meshData;

private slots:
    void onSurfacePatchRequest(double featureAngle);
    void onSurfaceCheckRequest();
};

#endif // MODEL_EDITOR_H