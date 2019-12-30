#include "JsonFormat.h"
#include <QDebug>

JsonFormat::JsonFormat(QObject *parent)
{
    Q_UNUSED(parent)
}

JsonFormat::~JsonFormat()
{
    if(m_jsonDocument!=nullptr){
        delete  m_jsonDocument;
        m_jsonDocument=nullptr;
    }
}

JsonModel *JsonFormat::jsonModel(){
    return m_jsonModel;
}

void JsonFormat::setJsonModel(JsonModel *jsonModel){
    this->m_jsonModel=jsonModel;
}


void JsonFormat::checkJonsStr(const QVariant data)
{
    QString jsonStr=data.toString();
    qDebug()<<jsonStr;
    const QByteArray jsonArray=jsonStr.toUtf8();
    m_jsonDocument=new QJsonDocument(QJsonDocument::fromJson(jsonArray,&m_error));
    if(m_error.error != QJsonParseError::NoError)
    {
        qDebug() << "json error!"<<m_error.errorString();
        qDebug()<<m_error.offset;
        return;
    }
    QByteArray result=m_jsonDocument->toJson(QJsonDocument::Indented);
    emit formattedJson(QString(result));
    delete m_jsonDocument;
}


void JsonFormat::convertJsonToTreeModel(const QVariant data)
{
    QString jsonStr=data.toString();
    qDebug()<<jsonStr;
    if(jsonStr==nullptr||jsonStr.isNull()||jsonStr.isEmpty()){
        return;
    }
    const QByteArray jsonArray=jsonStr.toUtf8();
    QJsonDocument *jsonDoc=new QJsonDocument(QJsonDocument::fromJson(jsonArray));
    if(m_error.error!=QJsonParseError::NoError)
    {
        qDebug() << "json error!"<<m_error.errorString();
        qDebug()<<m_error.offset;
        return;
    }
    if(jsonDoc==nullptr||jsonDoc->isEmpty()||jsonDoc->isNull()){
        qDebug()<<"jsondoc is empty";
        return;
    }
    m_jsonModel=new JsonModel();
    m_jsonModel->convertJsonToTree(jsonDoc);
    emit jsonModelChanged();
}

JsonModel::JsonModel(QObject *parent)
{
    Q_UNUSED(parent)
}

JsonModel::~JsonModel()
{
    delete rootItem;
}

int JsonModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant JsonModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        qDebug()<<"index inValid";
        return QVariant();
    }
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    switch (role) {
    case JsonType::KEY:
        return item->itemData();
    case JsonType::VALUE:
        return "value";
    }
    return QVariant();


}

Qt::ItemFlags JsonModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}


void JsonModel::convertJsonToTree(QJsonDocument *doc)
{
    rootItem=new TreeItem();
    rootItem->setData("JSON");
    if(doc->isObject()){
        QJsonObject jo=doc->object();
        parseJsonObject(rootItem,&jo);
    }

}

void JsonModel::parseJsonObject(TreeItem *parentItem, QJsonObject *jsonValue)
{
    QStringList keyList=jsonValue->keys();
    int count=keyList.size();
    for(int i=0;i<count;i++){
        QString key=keyList.at(i);
        QJsonValue jv = jsonValue->value(key);

        TreeItem * childItem=new TreeItem(rootItem);
        if(jv==QJsonValue::Undefined){
            childItem->setData(key+" : "+"null");
        }else if(jv.isBool()){
            childItem->setData(key+" : "+(jv.toBool()?"true":"false"));
        }else if(jv.isString()){
            childItem->setData(key+" : "+jv.toString());
        }else if(jv.isDouble())
        {
            childItem->setData(key+" : "+QString::number(jv.toDouble()));
        }
        else if (jv.isObject()) {
            childItem->setData(key);
            QJsonObject jo=jv.toObject();
            parseJsonObject(childItem,&jo);
        }else if (jv.isArray()) {
            childItem->setData(key);
            QJsonArray ja=jv.toArray();
            parseJsonArray(childItem,&ja);
        }else {
            qDebug()<<jv.toString();
            childItem->setData(key+":"+jv.toString("null"));
        }
        parentItem->appendChild(childItem);
    }
}

void JsonModel::parseJsonArray(TreeItem *parentItem, QJsonArray *jsonValue)
{
    int count=jsonValue->size();
    for(int i=0;i<count;i++){
        QJsonValue jv=jsonValue->at(i);
        if(jv==QJsonValue::Undefined){
            continue;
        }
        TreeItem * childItem=new TreeItem(rootItem);
        if(jv.isBool())
        {
            childItem->setData(QString::number(i)+" : "+(jv.toBool()?"true":"false"));
        }
        else if(jv.isString()){
            childItem->setData(QString::number(i)+" : "+jv.toString());
        }
        else if (jv.isDouble()) {
            childItem->setData(QString::number(i)+" : "+QString::number(jv.toDouble()));
        }
        else if(jv.isObject()){
            childItem->setData(QString::number(i)+" : ");
            QJsonObject jo=jv.toObject();
            parseJsonObject(childItem,&jo);
        }else if(jv.isArray()){
            QJsonArray ja=jv.toArray();
            childItem->setData(QString::number(i)+" : ");
            parseJsonArray(childItem,&ja);
        }
        parentItem->appendChild(childItem);
    }


}

QModelIndex JsonModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex JsonModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int JsonModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}



QHash<int, QByteArray> JsonModel::roleNames() const
{
    QHash<int, QByteArray>  d;
    d[JsonType::KEY] = "key";
    d[JsonType::VALUE] = "value";
    return d;
}
