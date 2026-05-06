#include "ui/PropertyPanel.h"

#include "occt/OcctUtils.h"

#include <QTextBrowser>
#include <QVBoxLayout>

PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    m_text = new QTextBrowser(this);
    m_text->setReadOnly(true);
    m_text->setPlaceholderText(tr("Select a shape in the tree."));
    layout->addWidget(m_text);
}

void PropertyPanel::showShape(const ShapeDocument& document, int shapeId)
{
    if (!m_text)
    {
        return;
    }
    if (shapeId < 0)
    {
        m_text->clear();
        return;
    }
    const ShapeNode* n = document.FindNode(shapeId);
    if (!n)
    {
        m_text->setPlainText(tr("Invalid shape id."));
        return;
    }

    QString html;
    html += QStringLiteral("<b>%1</b><br/>").arg(QString::fromStdString(n->name));
    html += QStringLiteral("Kind: %1<br/>").arg(QString::fromLatin1(ShapeKindDisplayName(n->kind)));
    html += QStringLiteral("Tolerance: %1<br/>").arg(n->tolerance, 0, 'g', 12);
    html += QStringLiteral("Null: %1<br/>").arg(n->isNull ? QStringLiteral("yes") : QStringLiteral("no"));
    html += QStringLiteral("Closed: %1<br/>").arg(n->isClosed ? QStringLiteral("yes") : QStringLiteral("no"));

    if (!n->bbox.IsVoid())
    {
        double xmin, ymin, zmin, xmax, ymax, zmax;
        n->bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        html += QStringLiteral("<br/><b>Bounding box</b><br/>");
        html += QStringLiteral("min: (%1, %2, %3)<br/>")
                    .arg(xmin, 0, 'g', 10)
                    .arg(ymin, 0, 'g', 10)
                    .arg(zmin, 0, 'g', 10);
        html += QStringLiteral("max: (%1, %2, %3)<br/>")
                    .arg(xmax, 0, 'g', 10)
                    .arg(ymax, 0, 'g', 10)
                    .arg(zmax, 0, 'g', 10);
    }
    else
    {
        html += QStringLiteral("<br/>Bounding box: void<br/>");
    }

    m_text->setHtml(html);
}
