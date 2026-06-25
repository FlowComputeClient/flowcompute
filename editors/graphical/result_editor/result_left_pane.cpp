#include "result_left_pane.h"
#include "color_bar_widget.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

ResultLeftPane::ResultLeftPane(const QStringList& timeFolders, QWidget* parent):
    m_timeFolders(timeFolders), QWidget(parent) {

    /*
    // Set stylesheet
    setStyleSheet(R"(
        QLabel         { color: black; }
        QComboBox      { color: black; }
        QSlider        { color: black; }
        QTableWidget   { color: black; }
        QHeaderView    { color: black; }
    )");
    */

    setProperty("widgetType", "pane");

    // Vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    setLayout(layout);

    // Label
    layout->addSpacing(5);
    QLabel* paneTitle = new QLabel(tr("<b>Simulation State</b>"));
    layout->addWidget(paneTitle, 0, Qt::AlignHCenter);
    layout->addSpacing(5);

    // --- Time Selection Controls ---

    // Create the combo label
    QHBoxLayout* comboLayout = new QHBoxLayout();
    comboLayout->setSpacing(10);
    comboLayout->setAlignment(Qt::AlignCenter);

    // Create and configure the label
    QLabel* timeLabel = new QLabel(tr("Time:"), this);
    comboLayout->addWidget(timeLabel);

    // Create and configure the combo box
    QComboBox* timeComboBox = new QComboBox(this);
    comboLayout->addWidget(timeComboBox);
    layout->addLayout(comboLayout);

    timeComboBox->addItems(m_timeFolders);

    // Create the slider
    QSlider* timeSlider = new QSlider(Qt::Vertical, this);
    timeSlider->setMinimum(0);
    int maxIndex = m_timeFolders.isEmpty() ? 0 : m_timeFolders.size() - 1;
    timeSlider->setMaximum(maxIndex);

    // Configure slider ticks
    timeSlider->setTickPosition(QSlider::TicksRight);
    if (maxIndex > 0) {
        int tickInterval = (maxIndex > 10) ? (maxIndex / 10) : 1;
        timeSlider->setTickInterval(tickInterval);
    }

    // Event-handling
    connect(timeSlider, &QSlider::valueChanged,
        timeComboBox, &QComboBox::setCurrentIndex);
    connect(timeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        timeSlider, &QSlider::setValue);
    if (maxIndex > 0) {
        timeSlider->setValue(maxIndex);
    }

    // A vertical slider usually looks best centered in its own layout
    QGridLayout* innerGrid = new QGridLayout();
    innerGrid->setContentsMargins(0, 5, 0, 5);
    innerGrid->setHorizontalSpacing(4);

    // Create the Min and Max labels
    QString maxTimeStr = m_timeFolders.isEmpty() ? "End" : m_timeFolders.last();
    QString minTimeStr = m_timeFolders.isEmpty() ? "Start" : m_timeFolders.first();

    QLabel* topLabel = new QLabel(maxTimeStr, this);
    QLabel* bottomLabel = new QLabel(minTimeStr, this);

    // Use a smaller font for the labels
    QFont smallFont = topLabel->font();
    smallFont.setPointSize(smallFont.pointSize() - 2);
    topLabel->setFont(smallFont);
    bottomLabel->setFont(smallFont);

    // Add widgets to the grid, pushing them toward the center seam
    innerGrid->addWidget(timeSlider, 0, 0, 3, 1, Qt::AlignRight);

    innerGrid->addWidget(topLabel, 0, 1, Qt::AlignBottom | Qt::AlignLeft);
    innerGrid->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 1);
    innerGrid->addWidget(bottomLabel, 2, 1, Qt::AlignTop | Qt::AlignLeft);

    // 2. Create an outer horizontal layout to act as a centering vice
    QHBoxLayout* centerLayout = new QHBoxLayout();
    centerLayout->addStretch();
    centerLayout->addLayout(innerGrid);
    centerLayout->addStretch();

    // 3. Add the perfectly centered group to your main vertical layout
    layout->addLayout(centerLayout);

    layout->addSpacing(10);

    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    layout->addSpacing(10);

    // --- Color Map Display ---
    QLabel* colorMapTitle = new QLabel(tr("<b>Normalized Pressure</b>"));
    layout->addWidget(colorMapTitle, 0, Qt::AlignHCenter);

    // Create color bar
    ColorBarWidget* colorBar = new ColorBarWidget(this);
    colorBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(colorBar, 0, Qt::AlignHCenter);
}