/* $Id: UIShortcutConfigurationEditor.cpp $ */
/** @file
 * VBox Qt GUI - UIShortcutConfigurationEditor class implementation.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QTabWidget>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIStyledItemDelegate.h"
#include "QITableView.h"
#include "UIActionPool.h"
#include "UICommon.h"
#include "UIExtraDataManager.h"
#include "UIHostComboEditor.h"
#include "UIHotKeyEditor.h"
#include "UIMessageCenter.h"
#include "UIShortcutConfigurationEditor.h"
#include "UIShortcutPool.h"

/* Namespaces: */
using namespace UIExtraDataDefs;


/** Table column indexes. */
enum TableColumnIndex
{
    TableColumnIndex_Description,
    TableColumnIndex_Sequence,
    TableColumnIndex_Max
};


/** QITableViewCell subclass for shortcut configuration editor. */
class UIShortcutTableViewCell : public QITableViewCell
{
    Q_OBJECT;

public:

    /** Constructs table cell on the basis of passed arguments.
      * @param  pParent  Brings the row this cell belongs too.
      * @param  strText  Brings the text describing this cell. */
    UIShortcutTableViewCell(QITableViewRow *pParent, const QString &strText)
        : QITableViewCell(pParent)
        , m_strText(strText)
    {}

    /** Returns the cell text. */
    virtual QString text() const RT_OVERRIDE { return m_strText; }

private:

    /** Holds the cell text. */
    QString m_strText;
};


/** QITableViewRow subclass for shortcut configuration editor. */
class UIShortcutTableViewRow : public QITableViewRow, public UIShortcutConfigurationItem
{
    Q_OBJECT;

public:

    /** Constructs table row on the basis of passed arguments.
      * @param  pParent  Brings the table this row belongs too.
      * @param  item     Brings the item this row is based on. */
    UIShortcutTableViewRow(QITableView *pParent = 0, const UIShortcutConfigurationItem &item = UIShortcutConfigurationItem())
        : QITableViewRow(pParent)
        , UIShortcutConfigurationItem(item)
    {
        createCells();
    }

    /** Constructs table row on the basis of @a another one. */
    UIShortcutTableViewRow(const UIShortcutTableViewRow &another)
        : QITableViewRow(another.table())
        , UIShortcutConfigurationItem(another)
    {
        createCells();
    }

    /** Destructs table row. */
    virtual ~UIShortcutTableViewRow() RT_OVERRIDE
    {
        destroyCells();
    }

    /** Copies a table row from @a another one. */
    UIShortcutTableViewRow &operator=(const UIShortcutTableViewRow &another)
    {
        /* Reassign variables: */
        setTable(another.table());
        UIShortcutConfigurationItem::operator=(another);

        /* Recreate cells: */
        destroyCells();
        createCells();

        /* Return this: */
        return *this;
    }

    /** Returns whether this row equals to @a another one. */
    bool operator==(const UIShortcutTableViewRow &another) const
    {
        /* Compare variables: */
        return UIShortcutConfigurationItem::operator==(another);
    }

protected:

    /** Returns the number of children. */
    virtual int childCount() const RT_OVERRIDE
    {
        return TableColumnIndex_Max;
    }

    /** Returns the child item with @a iIndex. */
    virtual QITableViewCell *childItem(int iIndex) const RT_OVERRIDE
    {
        switch (iIndex)
        {
            case TableColumnIndex_Description: return m_cells.first;
            case TableColumnIndex_Sequence: return m_cells.second;
            default: break;
        }
        return 0;
    }

private:

    /** Creates cells. */
    void createCells()
    {
        /* Create cells on the basis of description and current sequence: */
        m_cells = qMakePair(new UIShortcutTableViewCell(this, description()),
                            new UIShortcutTableViewCell(this, currentSequence()));
    }

    /** Destroys cells. */
    void destroyCells()
    {
        /* Destroy cells: */
        delete m_cells.first;
        delete m_cells.second;
        m_cells.first = 0;
        m_cells.second = 0;
    }

    /** Holds the cell instances. */
    QPair<UIShortcutTableViewCell*, UIShortcutTableViewCell*> m_cells;
};

/** Shortcut configuration editor row list. */
typedef QList<UIShortcutTableViewRow> UIShortcutTableViewContent;


/** Shortcut item sorting functor. */
class UIShortcutItemSortingFunctor
{
public:

    /** Constructs shortcut item sorting functor.
      * @param  iColumn     Brings the column sorting should be done according to.
      * @param  m_enmOrder  Brings the sorting order to be applied. */
    UIShortcutItemSortingFunctor(int iColumn, Qt::SortOrder enmOrder)
        : m_iColumn(iColumn)
        , m_enmOrder(enmOrder)
    {}

    /** Returns whether the @a item1 is more/less than the @a item2.
      * @note  Order depends on the one set through constructor, stored in m_enmOrder. */
    bool operator()(const UIShortcutTableViewRow &item1, const UIShortcutTableViewRow &item2)
    {
        switch (m_iColumn)
        {
            case TableColumnIndex_Description:
                return   m_enmOrder == Qt::AscendingOrder
                       ? item1.description() < item2.description()
                       : item1.description() > item2.description();
            case TableColumnIndex_Sequence:
                return   m_enmOrder == Qt::AscendingOrder
                       ? item1.currentSequence() < item2.currentSequence()
                       : item1.currentSequence() > item2.currentSequence();
            default:
                break;
        }
        return   m_enmOrder == Qt::AscendingOrder
               ? item1.key() < item2.key()
               : item1.key() > item2.key();
    }

private:

    /** Holds the column sorting should be done according to. */
    int            m_iColumn;
    /** Holds the sorting order to be applied. */
    Qt::SortOrder  m_enmOrder;
};


/** QAbstractTableModel subclass representing shortcut configuration model. */
class UIShortcutConfigurationModel : public QAbstractTableModel
{
    Q_OBJECT;

signals:

    /** Notifies about shortcuts loaded. */
    void sigShortcutsLoaded();
    /** Notifies about data changed. */
    void sigDataChanged();

public:

    /** Constructs model passing @a pParent to the base-class.
      * @param  enmType  Brings the action-pool type this model is related to. */
    UIShortcutConfigurationModel(QObject *pParent, UIActionPoolType enmType);

    /** Defines the parent @a pTable reference. */
    void setTable(UIShortcutConfigurationTable *pTable);

    /** Returns the number of children. */
    int childCount() const;
    /** Returns the child item with @a iIndex. */
    QITableViewRow *childItem(int iIndex);

    /** Loads a @a list of shortcuts to the model. */
    void load(const UIShortcutConfigurationList &list);
    /** Saves the model shortcuts to a @a list. */
    void save(UIShortcutConfigurationList &list);

    /** Returns whether all shortcuts unique. */
    bool isAllShortcutsUnique();

public slots:

    /** Handle filtering @a strText change. */
    void sltHandleFilterTextChange(const QString &strText);

protected:

    /** Returns the number of rows under the given @a parent. */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const RT_OVERRIDE;
    /** Returns the number of columns under the given @a parent. */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const RT_OVERRIDE;

    /** Returns the item flags for the given @a index. */
    virtual Qt::ItemFlags flags(const QModelIndex &index) const RT_OVERRIDE;
    /** Returns the data for the given @a iRole and @a iSection in the header with the specified @a enmOrientation. */
    virtual QVariant headerData(int iSection, Qt::Orientation enmOrientation, int iRole = Qt::DisplayRole) const RT_OVERRIDE;
    /** Returns the data stored under the given @a iRole for the item referred to by the @a index. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const RT_OVERRIDE;
    /** Sets the @a iRole data for the item at @a index to @a value. */
    virtual bool setData(const QModelIndex &index, const QVariant &value, int iRole = Qt::EditRole) RT_OVERRIDE;

    /** Sorts the model by @a iColumn in the given @a enmOrder. */
    virtual void sort(int iColumn, Qt::SortOrder enmOrder = Qt::AscendingOrder) RT_OVERRIDE;

private:

    /** Applies filter. */
    void applyFilter();

    /** Holds the action-pool type this model is related to. */
    UIActionPoolType  m_enmType;

    /** Holds the parent table reference. */
    UIShortcutConfigurationTable *m_pTable;

    /** Holds current filter. */
    QString  m_strFilter;

    /** Holds current shortcut list. */
    UIShortcutTableViewContent  m_shortcuts;
    /** Holds current filtered shortcut list. */
    UIShortcutTableViewContent  m_filteredShortcuts;

    /** Holds a set of currently duplicated sequences. */
    QSet<QString>  m_duplicatedSequences;
};


/** QITableView subclass representing shortcut configuration table. */
class UIShortcutConfigurationTable : public QITableView
{
    Q_OBJECT;

public:

    /** Constructs table passing @a pParent to the base-class.
      * @param  pModel         Brings the model this table is bound to.
      * @param  strObjectName  Brings the object name this table has, required for fast referencing. */
    UIShortcutConfigurationTable(QWidget *pParent, UIShortcutConfigurationModel *pModel, const QString &strObjectName);
    /** Destructs table. */
    virtual ~UIShortcutConfigurationTable() RT_OVERRIDE;

protected:

    /** Returns the number of children. */
    virtual int childCount() const RT_OVERRIDE;
    /** Returns the child item with @a iIndex. */
    virtual QITableViewRow *childItem(int iIndex) const RT_OVERRIDE;

private slots:

    /** Handles shortcuts loaded signal. */
    void sltHandleShortcutsLoaded();

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Holds the item editor factory instance. */
    QItemEditorFactory *m_pItemEditorFactory;
};


/*********************************************************************************************************************************
*   Class UIShortcutConfigurationModel implementation.                                                                           *
*********************************************************************************************************************************/

UIShortcutConfigurationModel::UIShortcutConfigurationModel(QObject *pParent, UIActionPoolType enmType)
    : QAbstractTableModel(pParent)
    , m_enmType(enmType)
    , m_pTable(0)
{
}

void UIShortcutConfigurationModel::setTable(UIShortcutConfigurationTable *pTable)
{
    m_pTable = pTable;
}

int UIShortcutConfigurationModel::childCount() const
{
    /* Return row count: */
    return rowCount();
}

QITableViewRow *UIShortcutConfigurationModel::childItem(int iIndex)
{
    /* Make sure index is within the bounds: */
    AssertReturn(iIndex >= 0 && iIndex < m_filteredShortcuts.size(), 0);
    /* Return corresponding filtered row: */
    return &m_filteredShortcuts[iIndex];
}

void UIShortcutConfigurationModel::load(const UIShortcutConfigurationList &list)
{
    /* Load a list of passed shortcuts: */
    foreach (const UIShortcutConfigurationItem &item, list)
    {
        /* Filter out unnecessary items: */
        if (   (m_enmType == UIActionPoolType_Manager && item.key().startsWith(GUI_Input_MachineShortcuts))
            || (m_enmType == UIActionPoolType_Runtime && item.key().startsWith(GUI_Input_SelectorShortcuts)))
            continue;
        /* Add suitable item to the model as a new shortcut: */
        m_shortcuts << UIShortcutTableViewRow(m_pTable, item);
    }
    /* Apply filter: */
    applyFilter();
    /* Notify table: */
    emit sigShortcutsLoaded();
}

void UIShortcutConfigurationModel::save(UIShortcutConfigurationList &list)
{
    /* Save cached model shortcuts: */
    foreach (const UIShortcutTableViewRow &row, m_shortcuts)
    {
        const UIShortcutConfigurationItem &item = row;

        /* Search for corresponding item position: */
        const int iShortcutItemPosition = UIShortcutSearchFunctor<UIShortcutConfigurationItem>()(list, item);
        /* Make sure position is valid: */
        if (iShortcutItemPosition == -1)
            continue;
        /* Save cached model shortcut to a list: */
        list[iShortcutItemPosition] = item;
    }
}

bool UIShortcutConfigurationModel::isAllShortcutsUnique()
{
    /* Enumerate all the sequences: */
    QMultiMap<QString, QString> usedSequences;
    foreach (const UIShortcutTableViewRow &item, m_shortcuts)
    {
        QString strKey = item.currentSequence();
        if (!strKey.isEmpty())
        {
            const QString strScope = item.scope();
            strKey = strScope.isNull() ? strKey : QString("%1: %2").arg(strScope, strKey);
            usedSequences.insert(strKey, item.key());
        }
    }
    /* Enumerate all the duplicated sequences: */
    QSet<QString> duplicatedSequences;
    foreach (const QString &strKey, usedSequences.keys())
        if (usedSequences.count(strKey) > 1)
        {
            foreach (const QString &strValue, usedSequences.values(strKey))
                duplicatedSequences |= strValue;
        }
    /* Is there something changed? */
    if (m_duplicatedSequences != duplicatedSequences)
    {
        m_duplicatedSequences = duplicatedSequences;
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
    /* Are there duplicated shortcuts? */
    if (!m_duplicatedSequences.isEmpty())
        return false;
    /* True by default: */
    return true;
}

void UIShortcutConfigurationModel::sltHandleFilterTextChange(const QString &strText)
{
    m_strFilter = strText;
    applyFilter();
}

int UIShortcutConfigurationModel::rowCount(const QModelIndex& /* parent = QModelIndex() */) const
{
    return m_filteredShortcuts.size();
}

int UIShortcutConfigurationModel::columnCount(const QModelIndex& /* parent = QModelIndex() */) const
{
    return TableColumnIndex_Max;
}

Qt::ItemFlags UIShortcutConfigurationModel::flags(const QModelIndex &index) const
{
    /* No flags for invalid index: */
    if (!index.isValid())
        return Qt::NoItemFlags;
    /* Switch for different columns: */
    switch (index.column())
    {
        case TableColumnIndex_Description: return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        case TableColumnIndex_Sequence: return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        default: break;
    }
    /* No flags by default: */
    return Qt::NoItemFlags;
}

QVariant UIShortcutConfigurationModel::headerData(int iSection,
                                                  Qt::Orientation enmOrientation,
                                                  int iRole /* = Qt::DisplayRole */) const
{
    /* Switch for different roles: */
    switch (iRole)
    {
        case Qt::DisplayRole:
        {
            /* Invalid for vertical header: */
            if (enmOrientation == Qt::Vertical)
                return QString();
            /* Switch for different columns: */
            switch (iSection)
            {
                case TableColumnIndex_Description: return UIShortcutConfigurationEditor::tr("Name");
                case TableColumnIndex_Sequence: return UIShortcutConfigurationEditor::tr("Shortcut");
                default: break;
            }
            /* Invalid for other cases: */
            return QString();
        }
        default:
            break;
    }
    /* Invalid by default: */
    return QVariant();
}

QVariant UIShortcutConfigurationModel::data(const QModelIndex &index, int iRole /* = Qt::DisplayRole */) const
{
    /* No data for invalid index: */
    if (!index.isValid())
        return QVariant();
    const int iIndex = index.row();
    /* Switch for different roles: */
    switch (iRole)
    {
        case Qt::DisplayRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case TableColumnIndex_Description:
                {
                    /* Return shortcut scope and description: */
                    const QString strScope = m_filteredShortcuts[iIndex].scope();
                    const QString strDescription = m_filteredShortcuts[iIndex].description();
                    return strScope.isNull() ? strDescription : QString("%1: %2").arg(strScope, strDescription);
                }
                case TableColumnIndex_Sequence:
                {
                    /* If that is host-combo cell: */
                    if (m_filteredShortcuts[iIndex].key() == UIHostCombo::hostComboCacheKey())
                        /* We should return host-combo: */
                        return UIHostCombo::toReadableString(m_filteredShortcuts[iIndex].currentSequence());
                    /* In other cases we should return hot-combo: */
                    QString strHotCombo = m_filteredShortcuts[iIndex].currentSequence();
                    /* But if that is machine table and hot-combo is not empty: */
                    if (m_enmType == UIActionPoolType_Runtime && !strHotCombo.isEmpty())
                        /* We should prepend it with Host+ prefix: */
                        strHotCombo.prepend(UIHostCombo::hostComboModifierName());
                    /* Return what we've got: */
                    return strHotCombo;
                }
                default: break;
            }
            /* Invalid for other cases: */
            return QString();
        }
        case Qt::EditRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case TableColumnIndex_Sequence:
                    return   m_filteredShortcuts[iIndex].key() == UIHostCombo::hostComboCacheKey()
                           ? QVariant::fromValue(UIHostComboWrapper(m_filteredShortcuts[iIndex].currentSequence()))
                           : QVariant::fromValue(UIHotKey(  m_enmType == UIActionPoolType_Runtime
                                                          ? UIHotKeyType_Simple
                                                          : UIHotKeyType_WithModifiers,
                                                          m_filteredShortcuts[iIndex].currentSequence(),
                                                          m_filteredShortcuts[iIndex].defaultSequence()));
                default:
                    break;
            }
            /* Invalid for other cases: */
            return QString();
        }
        case Qt::FontRole:
        {
            /* Do we have a default font? */
            QFont font(QApplication::font());
            /* Switch for different columns: */
            switch (index.column())
            {
                case TableColumnIndex_Sequence:
                {
                    if (   m_filteredShortcuts[iIndex].key() != UIHostCombo::hostComboCacheKey()
                        && m_filteredShortcuts[iIndex].currentSequence() != m_filteredShortcuts[iIndex].defaultSequence())
                        font.setBold(true);
                    break;
                }
                default: break;
            }
            /* Return resulting font: */
            return font;
        }
        case Qt::ForegroundRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case TableColumnIndex_Sequence:
                {
                    if (m_duplicatedSequences.contains(m_filteredShortcuts[iIndex].key()))
                        return QBrush(Qt::red);
                    break;
                }
                default: break;
            }
            /* Default for other cases: */
            return QString();
        }
        default: break;
    }
    /* Invalid by default: */
    return QVariant();
}

bool UIShortcutConfigurationModel::setData(const QModelIndex &index, const QVariant &value, int iRole /* = Qt::EditRole */)
{
    /* Nothing to set for invalid index: */
    if (!index.isValid())
        return false;
    /* Switch for different roles: */
    switch (iRole)
    {
        case Qt::EditRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case TableColumnIndex_Sequence:
                {
                    /* Get index: */
                    const int iIndex = index.row();
                    /* Set sequence to shortcut: */
                    UIShortcutTableViewRow &filteredShortcut = m_filteredShortcuts[iIndex];
                    const int iShortcutIndex = UIShortcutSearchFunctor<UIShortcutTableViewRow>()(m_shortcuts, filteredShortcut);
                    if (iShortcutIndex != -1)
                    {
                        filteredShortcut.setCurrentSequence(  filteredShortcut.key() == UIHostCombo::hostComboCacheKey()
                                                            ? value.value<UIHostComboWrapper>().toString()
                                                            : value.value<UIHotKey>().sequence());
                        m_shortcuts[iShortcutIndex] = filteredShortcut;
                        emit sigDataChanged();
                        return true;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    /* Nothing to set by default: */
    return false;
}

void UIShortcutConfigurationModel::sort(int iColumn, Qt::SortOrder order /* = Qt::AscendingOrder */)
{
    /* Sort whole the list: */
    std::stable_sort(m_shortcuts.begin(), m_shortcuts.end(), UIShortcutItemSortingFunctor(iColumn, order));
    /* Make sure host-combo item is always the first one: */
    UIShortcutConfigurationItem fakeHostComboItem(UIHostCombo::hostComboCacheKey(), QString(), QString(), QString(), QString());
    UIShortcutTableViewRow fakeHostComboTableViewRow(0, fakeHostComboItem);
    const int iIndexOfHostComboItem = UIShortcutSearchFunctor<UIShortcutTableViewRow>()(m_shortcuts, fakeHostComboTableViewRow);
    if (iIndexOfHostComboItem != -1)
    {
        UIShortcutTableViewRow hostComboItem = m_shortcuts.takeAt(iIndexOfHostComboItem);
        m_shortcuts.prepend(hostComboItem);
    }
    /* Apply the filter: */
    applyFilter();
    /* Notify the model: */
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void UIShortcutConfigurationModel::applyFilter()
{
    /* Erase items first if necessary: */
    if (!m_filteredShortcuts.isEmpty())
    {
        beginRemoveRows(QModelIndex(), 0, m_filteredShortcuts.size() - 1);
        m_filteredShortcuts.clear();
        endRemoveRows();
    }

    /* If filter is empty: */
    if (m_strFilter.isEmpty())
    {
        /* Just add all the items: */
        m_filteredShortcuts = m_shortcuts;
    }
    else
    {
        /* Check if the description matches the filter: */
        foreach (const UIShortcutTableViewRow &item, m_shortcuts)
        {
            /* If neither scope nor description or sequence matches the filter, skip item: */
            if (   !item.scope().contains(m_strFilter, Qt::CaseInsensitive)
                && !item.description().contains(m_strFilter, Qt::CaseInsensitive)
                && !item.currentSequence().contains(m_strFilter, Qt::CaseInsensitive))
                continue;
            /* Add that item: */
            m_filteredShortcuts << item;
        }
    }

    /* Add items finally if necessary: */
    if (!m_filteredShortcuts.isEmpty())
    {
        beginInsertRows(QModelIndex(), 0, m_filteredShortcuts.size() - 1);
        endInsertRows();
    }
}


/*********************************************************************************************************************************
*   Class UIShortcutConfigurationTable implementation.                                                                           *
*********************************************************************************************************************************/

UIShortcutConfigurationTable::UIShortcutConfigurationTable(QWidget *pParent,
                                                           UIShortcutConfigurationModel *pModel,
                                                           const QString &strObjectName)
    : QITableView(pParent)
    , m_pItemEditorFactory(0)
{
    /* Set object name: */
    setObjectName(strObjectName);
    /* Set model: */
    setModel(pModel);

    /* Prepare all: */
    prepare();
}

UIShortcutConfigurationTable::~UIShortcutConfigurationTable()
{
    /* Cleanup all: */
    cleanup();
}

int UIShortcutConfigurationTable::childCount() const
{
    /* Redirect request to table model: */
    return qobject_cast<UIShortcutConfigurationModel*>(model())->childCount();
}

QITableViewRow *UIShortcutConfigurationTable::childItem(int iIndex) const
{
    /* Redirect request to table model: */
    return qobject_cast<UIShortcutConfigurationModel*>(model())->childItem(iIndex);
}

void UIShortcutConfigurationTable::sltHandleShortcutsLoaded()
{
    /* Resize columns to feat contents: */
    resizeColumnsToContents();

    /* Configure sorting: */
    sortByColumn(TableColumnIndex_Description, Qt::AscendingOrder);
    setSortingEnabled(true);
}

void UIShortcutConfigurationTable::prepare()
{
    /* Configure self: */
    setTabKeyNavigation(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked);

    /* Configure headers: */
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize((int)(verticalHeader()->minimumSectionSize() * 1.33));
    horizontalHeader()->setStretchLastSection(false);
    horizontalHeader()->setSectionResizeMode(TableColumnIndex_Description, QHeaderView::Interactive);
    horizontalHeader()->setSectionResizeMode(TableColumnIndex_Sequence, QHeaderView::Stretch);

    /* Connect model: */
    UIShortcutConfigurationModel *pHotKeyTableModel = qobject_cast<UIShortcutConfigurationModel*>(model());
    if (pHotKeyTableModel)
        connect(pHotKeyTableModel, &UIShortcutConfigurationModel::sigShortcutsLoaded,
                this, &UIShortcutConfigurationTable::sltHandleShortcutsLoaded);

    /* Check if we do have proper item delegate: */
    QIStyledItemDelegate *pStyledItemDelegate = qobject_cast<QIStyledItemDelegate*>(itemDelegate());
    if (pStyledItemDelegate)
    {
        /* Configure item delegate: */
        pStyledItemDelegate->setWatchForEditorDataCommits(true);

        /* Create new item editor factory: */
        m_pItemEditorFactory = new QItemEditorFactory;
        if (m_pItemEditorFactory)
        {
            /* Register UIHotKeyEditor as the UIHotKey editor: */
            int iHotKeyTypeId = qRegisterMetaType<UIHotKey>();
            QStandardItemEditorCreator<UIHotKeyEditor> *pHotKeyItemEditorCreator = new QStandardItemEditorCreator<UIHotKeyEditor>();
            m_pItemEditorFactory->registerEditor((QVariant::Type)iHotKeyTypeId, pHotKeyItemEditorCreator);

            /* Register UIHostComboEditor as the UIHostComboWrapper editor: */
            int iHostComboTypeId = qRegisterMetaType<UIHostComboWrapper>();
            QStandardItemEditorCreator<UIHostComboEditor> *pHostComboItemEditorCreator = new QStandardItemEditorCreator<UIHostComboEditor>();
            m_pItemEditorFactory->registerEditor((QVariant::Type)iHostComboTypeId, pHostComboItemEditorCreator);

            /* Assign configured item editor factory to item delegate: */
            pStyledItemDelegate->setItemEditorFactory(m_pItemEditorFactory);
        }
    }
}

void UIShortcutConfigurationTable::cleanup()
{
    /* Cleanup item editor factory: */
    delete m_pItemEditorFactory;
    m_pItemEditorFactory = 0;
}


/*********************************************************************************************************************************
*   Class UIShortcutConfigurationEditor implementation.                                                                          *
*********************************************************************************************************************************/

UIShortcutConfigurationEditor::UIShortcutConfigurationEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pModelManager(0)
    , m_pModelRuntime(0)
    , m_pTabWidget(0)
    , m_pEditorFilterManager(0), m_pTableManager(0)
    , m_pEditorFilterRuntime(0), m_pTableRuntime(0)
{
    prepare();
}

void UIShortcutConfigurationEditor::load(const UIShortcutConfigurationList &value)
{
    m_pModelManager->load(value);
    m_pModelRuntime->load(value);
}

void UIShortcutConfigurationEditor::save(UIShortcutConfigurationList &value) const
{
    m_pModelManager->save(value);
    m_pModelRuntime->save(value);
}

bool UIShortcutConfigurationEditor::isShortcutsUniqueManager() const
{
    return m_pModelManager->isAllShortcutsUnique();
}

bool UIShortcutConfigurationEditor::isShortcutsUniqueRuntime() const
{
    return m_pModelRuntime->isAllShortcutsUnique();
}

QString UIShortcutConfigurationEditor::tabNameManager() const
{
    return m_pTabWidget->tabText(TableIndex_Manager);
}

QString UIShortcutConfigurationEditor::tabNameRuntime() const
{
    return m_pTabWidget->tabText(TableIndex_Runtime);
}

void UIShortcutConfigurationEditor::retranslateUi()
{
    m_pTabWidget->setTabText(TableIndex_Manager, tr("&VirtualBox Manager"));
    m_pTabWidget->setTabText(TableIndex_Runtime, tr("Virtual &Machine"));
    m_pTableManager->setWhatsThis(tr("Lists all available shortcuts which can be configured."));
    m_pTableRuntime->setWhatsThis(tr("Lists all available shortcuts which can be configured."));
    m_pEditorFilterManager->setToolTip(tr("Holds a sequence to filter the shortcut list."));
    m_pEditorFilterRuntime->setToolTip(tr("Holds a sequence to filter the shortcut list."));
}

void UIShortcutConfigurationEditor::prepare()
{
    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();
}

void UIShortcutConfigurationEditor::prepareWidgets()
{
    /* Prepare main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    if (pMainLayout)
    {
        pMainLayout->setContentsMargins(0, 0, 0, 0);

        /* Prepare tab-widget: */
        m_pTabWidget = new QTabWidget(this);
        if (m_pTabWidget)
        {
            /* Prepare 'Manager UI' tab: */
            prepareTabManager();
            /* Prepare 'Runtime UI' tab: */
            prepareTabRuntime();

            /* Add tab-widget into layout: */
            pMainLayout->addWidget(m_pTabWidget);
        }
    }
}

void UIShortcutConfigurationEditor::prepareTabManager()
{
    /* Prepare Manager UI tab: */
    QWidget *pTabManager = new QWidget;
    if (pTabManager)
    {
        /* Prepare Manager UI layout: */
        QVBoxLayout *pLayoutManager = new QVBoxLayout(pTabManager);
        if (pLayoutManager)
        {
            pLayoutManager->setSpacing(1);
#ifdef VBOX_WS_MAC
            /* On Mac OS X and X11 we can do a bit of smoothness: */
            pLayoutManager->setContentsMargins(0, 0, 0, 0);
#endif

            /* Prepare Manager UI filter editor: */
            m_pEditorFilterManager = new QLineEdit(pTabManager);
            if (m_pEditorFilterManager)
                pLayoutManager->addWidget(m_pEditorFilterManager);

            /* Prepare Manager UI model: */
            m_pModelManager = new UIShortcutConfigurationModel(this, UIActionPoolType_Manager);

            /* Prepare Manager UI table: */
            m_pTableManager = new UIShortcutConfigurationTable(pTabManager, m_pModelManager, "m_pTableManager");
            if (m_pTableManager)
            {
                m_pModelManager->setTable(m_pTableManager);
                pLayoutManager->addWidget(m_pTableManager);
            }
        }

        m_pTabWidget->insertTab(TableIndex_Manager, pTabManager, QString());
    }
}

void UIShortcutConfigurationEditor::prepareTabRuntime()
{
    /* Create Runtime UI tab: */
    QWidget *pTabMachine = new QWidget;
    if (pTabMachine)
    {
        /* Prepare Runtime UI layout: */
        QVBoxLayout *pLayoutMachine = new QVBoxLayout(pTabMachine);
        if (pLayoutMachine)
        {
            pLayoutMachine->setSpacing(1);
#ifdef VBOX_WS_MAC
            /* On Mac OS X and X11 we can do a bit of smoothness: */
            pLayoutMachine->setContentsMargins(0, 0, 0, 0);
#endif

            /* Prepare Runtime UI filter editor: */
            m_pEditorFilterRuntime = new QLineEdit(pTabMachine);
            if (m_pEditorFilterRuntime)
                pLayoutMachine->addWidget(m_pEditorFilterRuntime);

            /* Prepare Runtime UI model: */
            m_pModelRuntime = new UIShortcutConfigurationModel(this, UIActionPoolType_Runtime);

            /* Create Runtime UI table: */
            m_pTableRuntime = new UIShortcutConfigurationTable(pTabMachine, m_pModelRuntime, "m_pTableRuntime");
            if (m_pTableRuntime)
            {
                m_pModelRuntime->setTable(m_pTableRuntime);
                pLayoutMachine->addWidget(m_pTableRuntime);
            }
        }

        m_pTabWidget->insertTab(TableIndex_Runtime, pTabMachine, QString());

        /* In the VM process we start by displaying the Runtime UI tab: */
        if (uiCommon().uiType() == UICommon::UIType_RuntimeUI)
            m_pTabWidget->setCurrentWidget(pTabMachine);
    }
}

void UIShortcutConfigurationEditor::prepareConnections()
{
    /* Configure 'Manager UI' connections: */
    connect(m_pEditorFilterManager, &QLineEdit::textChanged,
            m_pModelManager, &UIShortcutConfigurationModel::sltHandleFilterTextChange);
    connect(m_pModelManager, &UIShortcutConfigurationModel::sigDataChanged,
            this, &UIShortcutConfigurationEditor::sigValueChanged);

    /* Configure 'Runtime UI' connections: */
    connect(m_pEditorFilterRuntime, &QLineEdit::textChanged,
            m_pModelRuntime, &UIShortcutConfigurationModel::sltHandleFilterTextChange);
    connect(m_pModelRuntime, &UIShortcutConfigurationModel::sigDataChanged,
            this, &UIShortcutConfigurationEditor::sigValueChanged);
}

# include "UIShortcutConfigurationEditor.moc"
