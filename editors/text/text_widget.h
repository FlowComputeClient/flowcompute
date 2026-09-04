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

#ifndef EDITORS_TEXT_TEXT_WIDGET_H_
#define EDITORS_TEXT_TEXT_WIDGET_H_

#include <QWidget>

#include "editors/text/file_changed_banner.h"
#include "editors/text/text_editor.h"

class TextWidget : public QWidget  {
    Q_OBJECT
 public:
    explicit TextWidget(QWidget *parent = nullptr);

    QTextDocument* document() const { return m_textEditor->document(); }
    TextEditor* editor() const { return m_textEditor; }
    void showBanner() { m_banner->show(); }
    void hideBanner() { m_banner->hide(); }
    void setTextData(const QByteArray &d) { m_textEditor->setTextData(d); }

 signals:
    void reloadRequested();

 private:
    FileChangedBanner* m_banner;
    TextEditor* m_textEditor;
};

#endif // EDITORS_TEXT_TEXT_WIDGET_H_
