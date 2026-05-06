#include "ui/ShapeTreeWidget.h"

#include <QHash>
#include <QHeaderView>

ShapeTreeWidget::ShapeTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabels(QStringList() << QStringLiteral("Shape"));
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(
        this,
        &QTreeWidget::currentItemChanged,
        this,
        &ShapeTreeWidget::onCurrentItemChanged);
}

void ShapeTreeWidget::rebuildFromDocument(const ShapeDocument& document)
{
    clear();
    m_idToItem.clear();

    for (const ShapeNode& n : document.Nodes())
    {
        auto* item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString(n.name));
        item->setData(0, Qt::UserRole, n.id);
        m_idToItem.insert(n.id, item);
    }

    for (const ShapeNode& n : document.Nodes())
    {
        QTreeWidgetItem* item = m_idToItem.value(n.id, nullptr);
        if (!item)
        {
            continue;
        }
        if (n.parentId < 0)
        {
            addTopLevelItem(item);
        }
        else
        {
            QTreeWidgetItem* p = m_idToItem.value(n.parentId, nullptr);
            if (p)
            {
                p->addChild(item);
            }
            else
            {
                addTopLevelItem(item);
            }
        }
    }

    expandToDepth(2);
}

void ShapeTreeWidget::selectShapeId(int shapeId)
{
    QTreeWidgetItem* item = m_idToItem.value(shapeId, nullptr);
    if (!item)
    {
        return;
    }
    setCurrentItem(item);
    scrollToItem(item);
}

void ShapeTreeWidget::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem*)
{
    if (!current)
    {
        emit shapeSelected(-1);
        return;
    }
    const QVariant v = current->data(0, Qt::UserRole);
    emit shapeSelected(v.isValid() ? v.toInt() : -1);
}
