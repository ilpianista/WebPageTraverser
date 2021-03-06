/*
 *   This file is part of WebPageTraverser.
 *
 *   Copyright 2012-2014 Andrea Scarpino <me@andreascarpino.it>
 *
 *   WebPageTraverser is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   WebPageTraverser is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with WebPageTraverser.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pagetraverser.h"

#include <QTextCodec>
#include <QUrl>
#include <QWebFrame>
#include <QNetworkReply>

PageTraverser::PageTraverser(QObject *parent) :
    QObject(parent),
    page(new CustomWebPage()),
    loop(new QEventLoop())
{
    page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);

    connect(&page, SIGNAL(loadFinished(bool)), this, SLOT(extractElements(bool)));
    connect(page.networkAccessManager(), SIGNAL(finished(QNetworkReply *)),
            this, SLOT(httpResponse(QNetworkReply *)));
    connect(this, SIGNAL(fetched()), &loop, SLOT(quit()));
}

PageTraverser::~PageTraverser()
{
    delete root;
}

WebElement* PageTraverser::traverse(const QString &url)
{
    //load the page
    QWebFrame *frame = page.mainFrame();
    frame->load(QUrl(url));

    //loop until the fetched signal is emitted
    loop.exec();

    return root;
}

void PageTraverser::extractElements(const bool ok)
{
    //qDebug() << "Loaded webpage: " << page.mainFrame()->url().toString() << "\n";

    QWebFrame *frame = page.mainFrame();
    if (ok) {
        const QWebElement doc(frame->documentElement());
        const QWebElement head(doc.firstChild());
        const QWebElement body(head.nextSibling());

        root = populateTree("html", "html", "NO-CSS", body);
    }

    // we've done here
    emit fetched();
}

void PageTraverser::httpResponse(QNetworkReply *reply)
{
    switch (reply->error()) {
    case QNetworkReply::NoError:
        return;
    case QNetworkReply::ContentNotFoundError:
        qCritical() << "Got a bad HTTP status code: " <<
                       reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        exit(1);
        break;
    case QNetworkReply::TimeoutError:
        qCritical() << "Request timeout.";
        exit(1);
        break;
    default:
        qCritical() << "Got an error during request:" << reply->errorString();
        exit(1);
    }
}

WebElement* PageTraverser::populateTree(const QString &parentPath,
                                        const QString &parentDomCSSPath,
                                        const QString &parentCSSPath,
                                        const QWebElement &element)
{
    //position
    Position position;
    position.top = element.geometry().top();
    position.left = element.geometry().left();
    position.right = element.geometry().right();
    position.bottom = element.geometry().bottom();

    //size
    Size size;
    size.height = element.geometry().height();
    size.width = element.geometry().width();

    //attributes
    QHash<QString, QString> attributes;
    Q_FOREACH (const QString attr, element.attributeNames()) {
        attributes.insert(attr, element.attribute(attr));
    }

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    const QString nodeTag(element.tagName().toLower());
    //create the web Element
    WebElement *node = new WebElement(parentPath, parentDomCSSPath, parentCSSPath,
                                      nodeTag, position, size, attributes,
                                      element.toPlainText().trimmed());

    //add its children tree
    for (QWebElement elem = element.firstChild(); !elem.isNull(); elem = elem.nextSibling()) {
        if (elem.attribute("class").isNull()) {
            node->getChildren()->append(populateTree(parentPath + "/" + nodeTag,
                                                     parentDomCSSPath + "/" + nodeTag,
                                                     parentCSSPath + "/NO-CSS", elem));
        } else {
            node->getChildren()->append(populateTree(parentPath + "/" + nodeTag,
                                                     parentDomCSSPath + "/" + nodeTag + ":" + elem.attribute("class"),
                                                     parentDomCSSPath + "/" + elem.attribute("class"), elem));
        }
    }

    return node;
}
