#include "locationcompleter.h"
#include "locationbar.h"

LocationCompleter::LocationCompleter(QObject *parent) :
    QCompleter(parent)
{
    setMaxVisibleItems(6);
    QStandardItemModel* completeModel = new QStandardItemModel();

    setModel(completeModel);
    QTreeView *treeView = new QTreeView;

    setPopup(treeView);
    treeView->setRootIsDecorated(false);
    treeView->header()->hide();
    treeView->header()->setStretchLastSection(false);
    treeView->header()->setResizeMode(0, QHeaderView::Stretch);
    treeView->header()->resizeSection(1,0);

    setCompletionMode(QCompleter::PopupCompletion);
    setCaseSensitivity(Qt::CaseInsensitive);
    setWrapAround(true);
    setCompletionColumn(1);
    QTimer::singleShot(0, this, SLOT(loadInitialHistory()));
}

//QString LocationCompleter::pathFromIndex(const QModelIndex &index) const
//{
//    qDebug() << __FUNCTION__ << "called";
//    return QCompleter::pathFromIndex(index);
//}

QStringList LocationCompleter::splitPath(const QString &path) const
{
    Q_UNUSED(path);
    return QStringList("");
#if 0
    QStringList returned = QCompleter::splitPath(path);
    QStringList returned2;
    QSqlQuery query;
    query.exec("SELECT url FROM history WHERE title LIKE '%"+path+"%' OR url LIKE '%"+path+"%' ORDER BY count DESC LIMIT 1");
    if (query.next()) {
        QString url = query.value(0).toString();
        bool titleSearching = false;
        if (!url.contains(path))
            titleSearching = true;
        QString prefix = url.mid(0,url.indexOf(path));
        foreach (QString string, returned) {
            if (titleSearching)
                returned2.append(url);
            else
                returned2.append(prefix+string);
        }
        return returned2;
    } else {
        foreach (QString string, returned)
            returned2.append("http://www.google.com/search?client=qupzilla&q=" + string);
        return returned2;
    }
#endif
}

void LocationCompleter::loadInitialHistory()
{
    QSqlQuery query;
    query.exec("SELECT title, url FROM history LIMIT 5");
    int i = 0;
    QStandardItemModel* cModel = (QStandardItemModel*)model();

    while(query.next()) {
        QStandardItem* iconText = new QStandardItem();
        QStandardItem* findUrl = new QStandardItem();
        QString url = query.value(1).toUrl().toEncoded();

        iconText->setIcon(LocationBar::icon(query.value(1).toUrl()).pixmap(16,16));
        iconText->setText(query.value(0).toString().replace("\n","").append("\n"+url));

        findUrl->setText(url);
        QList<QStandardItem*> items;
        items.append(iconText);
        items.append(findUrl);
        cModel->insertRow(i, items);
        i++;
    }
}

void LocationCompleter::refreshCompleter(QString string)
{
    QSqlQuery query;
    int limit;
    if (string.size() < 3)
        limit = 25;
    else
        limit = 15;

    query.exec("SELECT title, url FROM history WHERE title LIKE '%"+string+"%' OR url LIKE '%"+string+"%' ORDER BY count DESC LIMIT "+QString::number(limit));
    int i = 0;
    QStandardItemModel* cModel = (QStandardItemModel*)model();
    QTreeView *treeView = (QTreeView*)popup();

    cModel->clear();
    while(query.next()) {
        QStandardItem* iconText = new QStandardItem();
        QStandardItem* findUrl = new QStandardItem();
        QString url = query.value(1).toUrl().toEncoded();

        iconText->setIcon(LocationBar::icon(query.value(1).toUrl()).pixmap(16,16));
        iconText->setText(query.value(0).toString().replace("\n","").append("\n"+url));

        findUrl->setText(url);
        QList<QStandardItem*> items;
        items.append(iconText);
        items.append(findUrl);
        cModel->insertRow(i, items);
        i++;
    }

    if (i == 0) {
        QStandardItem* iconText = new QStandardItem();
        QStandardItem* findUrl = new QStandardItem();
        QString url("http://www.google.com/search?client=qupzilla&q="+string);

        iconText->setIcon(QIcon(":/icons/menu/google.png"));
        iconText->setText(tr("Search %1 on Google.com\n..........").arg(string));
        findUrl->setText(url);
        QList<QStandardItem*> items;
        items.append(iconText);
        items.append(findUrl);
        cModel->insertRow(i, items);
    }

    treeView->header()->setResizeMode(0, QHeaderView::Stretch);
    treeView->header()->resizeSection(1,0);

    if (i>6)
        popup()->setMinimumHeight(190);
    else
        popup()->setMinimumHeight(0);
    popup()->setUpdatesEnabled(true);
}