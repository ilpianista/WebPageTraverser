#include "pagetraverser.h"

// Qt headers
#include <QtCore/QQueue>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtWebKit/QWebFrame>

PageTraverser::PageTraverser(QObject *parent) :
    QObject(parent)
{
    page = new QWebPage();
    loop = new QEventLoop();

    connect(page, SIGNAL(loadFinished(bool)), this, SLOT(extractElements()));
    connect(this, SIGNAL(fetched()), loop, SLOT(quit()));
}

PageTraverser::~PageTraverser()
{
    delete &page;
    delete &loop;
    delete &root;
}

 WebElement* PageTraverser::traverse(const QString &url)
 {
    //load the page
    QWebFrame *frame = page->mainFrame();
    frame->load(QUrl(url));

    //wait for page loaded signal
    loop->exec();

    return root;
 }

 void PageTraverser::extractElements()
 {
//     QTextStream qout(stdout);
//     qout << "Loaded webpage: " << page->mainFrame()->url().toString() << "\n";
//     qout.flush();

     QWebFrame *frame = page->mainFrame();
     QWebElement doc = frame->documentElement();
     QWebElement head = doc.firstChild();
     QWebElement body = head.nextSibling();

     root =  populateTree("body",body);

     emit fetched();
 }

 WebElement* PageTraverser::populateTree(const QString parentPath, const QWebElement &e)
 {
    //parent path
    QString path = parentPath + "/" + e.tagName();

    //position
    Position position;
    position.m_top = e.geometry().top();
    position.m_left = e.geometry().left();
    position.m_right = e.geometry().right();
    position.m_bottom = e.geometry().bottom();

    //size
    Size size;
    size.m_height = e.geometry().height();
    size.m_width = e.geometry().width();

    //attributes
    QHash<QString, QString> attributes;
    if (!e.attributeNames().isEmpty()) {
        foreach (QString attr, e.attributeNames()) {
            attributes.insert(attr, e.attribute(attr));
        }
    }

    //create web Element
    WebElement* node = new WebElement(path,e.tagName(),position, size,attributes);

     //per ogni figlio
     //lista dei figli add populateTree(figlio);
    for (QWebElement elem = e.firstChild(); !elem.isNull(); elem = elem.nextSibling()) {
        node->getChildren()->append(populateTree(path,elem));
    }

    return node;
 }











