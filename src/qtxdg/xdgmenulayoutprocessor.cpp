/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "xdgmenulayoutprocessor.h"
#include "xmlhelper.h"
#include <QDebug>
#include <QCollator>
#include <QList>

using namespace Qt::Literals::StringLiterals;

// Helper functions prototypes
QDomElement findLastElementByTag(const QDomElement &element, const QString &tagName);
int childsCount(const QDomElement& element);


QDomElement findLastElementByTag(const QDomElement &element, const QString &tagName)
{
    QDomNodeList l = element.elementsByTagName(tagName);
    if (l.isEmpty())
        return QDomElement();

    return l.at(l.length()-1).toElement();
}


/************************************************
 If no default-layout has been specified then the layout as specified by
 the following elements should be assumed:
 <DefaultLayout
     show_empty="false"
     inline="false"
     inline_limit="4"
     inline_header="true"
     inline_alias="false">
     <Merge type="menus"/>
     <Merge type="files"/>
 </DefaultLayout>
 ************************************************/
XdgMenuLayoutProcessor::XdgMenuLayoutProcessor(QDomElement& element):
    mElement(element),
    mDefaultLayout(findLastElementByTag(element, "DefaultLayout"_L1))
{
    mDefaultParams.mShowEmpty = false;
    mDefaultParams.mInline = false;
    mDefaultParams.mInlineLimit = 4;
    mDefaultParams.mInlineHeader = true;
    mDefaultParams.mInlineAlias = false;

    if (mDefaultLayout.isNull())
    {
        // Create DefaultLayout node
        QDomDocument doc = element.ownerDocument();
        mDefaultLayout = doc.createElement("DefaultLayout"_L1);

        QDomElement menus = doc.createElement("Merge"_L1);
        menus.setAttribute("type"_L1, "menus"_L1);
        mDefaultLayout.appendChild(menus);

        QDomElement files = doc.createElement("Merge"_L1);
        files.setAttribute("type"_L1, "files"_L1);
        mDefaultLayout.appendChild(files);

        mElement.appendChild(mDefaultLayout);
    }

    setParams(mDefaultLayout, &mDefaultParams);

    // If a menu does not contain a <Layout> element or if it contains an empty <Layout> element
    // then the default layout should be used.
    mLayout = findLastElementByTag(element, "Layout"_L1);
    if (mLayout.isNull() || !mLayout.hasChildNodes())
        mLayout = mDefaultLayout;
}


XdgMenuLayoutProcessor::XdgMenuLayoutProcessor(QDomElement& element, XdgMenuLayoutProcessor *parent):
	mDefaultParams(parent->mDefaultParams),
    mElement(element)
{
    // DefaultLayout ............................
    QDomElement defaultLayout = findLastElementByTag(element, "DefaultLayout"_L1);

    if (defaultLayout.isNull())
        mDefaultLayout = parent->mDefaultLayout;
    else
        mDefaultLayout = defaultLayout;

    setParams(mDefaultLayout, &mDefaultParams);

    // If a menu does not contain a <Layout> element or if it contains an empty <Layout> element
    // then the default layout should be used.
    mLayout = findLastElementByTag(element, "Layout"_L1);
    if (mLayout.isNull() || !mLayout.hasChildNodes())
        mLayout = mDefaultLayout;

}


void XdgMenuLayoutProcessor::setParams(QDomElement defaultLayout, LayoutParams *result)
{
    if (defaultLayout.hasAttribute("show_empty"_L1))
        result->mShowEmpty = defaultLayout.attribute("show_empty"_L1) == "true"_L1;

    if (defaultLayout.hasAttribute("inline"_L1))
        result->mInline = defaultLayout.attribute("inline"_L1) == "true"_L1;

    if (defaultLayout.hasAttribute("inline_limit"_L1))
        result->mInlineLimit = defaultLayout.attribute("inline_limit"_L1).toInt();

    if (defaultLayout.hasAttribute("inline_header"_L1))
        result->mInlineHeader = defaultLayout.attribute("inline_header"_L1) == "true"_L1;

    if (defaultLayout.hasAttribute("inline_alias"_L1))
        result->mInlineAlias = defaultLayout.attribute("inline_alias"_L1) == "true"_L1;
}


QDomElement XdgMenuLayoutProcessor::searchElement(const QString &tagName, const QString &attributeName, const QString &attributeValue) const
{
    DomElementIterator it(mElement, tagName);
    while (it.hasNext())
    {
        QDomElement e = it.next();
        if (e.attribute(attributeName) == attributeValue)
        {
            return e;
        }
    }

    return QDomElement();
}


int childsCount(const QDomElement& element)
{
    int count = 0;
    DomElementIterator it(element);
    while (it.hasNext())
    {
        QString tag = it.next().tagName();
        if (tag == "AppLink"_L1 || tag == "Menu"_L1 || tag == "Separator"_L1)
            count ++;
    }

    return count;
}


void XdgMenuLayoutProcessor::run()
{
    QDomDocument doc = mLayout.ownerDocument();
    mResult = doc.createElement("Result"_L1);
    mElement.appendChild(mResult);

    // Process childs menus ...............................
    {
        DomElementIterator it(mElement, "Menu"_L1);
        while (it.hasNext())
        {
            QDomElement e = it.next();
            XdgMenuLayoutProcessor p(e, this);
            p.run();
        }
    }


    // Step 1 ...................................
    DomElementIterator it(mLayout);
    it.toFront();
    while (it.hasNext())
    {
        QDomElement e = it.next();

        if (e.tagName() == "Filename"_L1)
            processFilenameTag(e);

        else if (e.tagName() == "Menuname"_L1)
            processMenunameTag(e);

        else if (e.tagName() == "Separator"_L1)
            processSeparatorTag(e);

        else if (e.tagName() == "Merge"_L1)
        {
            QDomElement merge = mResult.ownerDocument().createElement("Merge"_L1);
            merge.setAttribute("type"_L1, e.attribute("type"_L1));
            mResult.appendChild(merge);
        }
    }

    // Step 2 ...................................
    {
        MutableDomElementIterator ri(mResult, "Merge"_L1);
        while (ri.hasNext())
        {
            processMergeTag(ri.next());
        }
    }

    // Move result cilds to element .............
    MutableDomElementIterator ri(mResult);
    while (ri.hasNext())
    {
        mElement.appendChild(ri.next());
    }

    // Final ....................................
    mElement.removeChild(mResult);

    if (mLayout.parentNode() == mElement)
        mElement.removeChild(mLayout);

    if (mDefaultLayout.parentNode() == mElement)
        mElement.removeChild(mDefaultLayout);

}


/************************************************
 The <Filename> element is the most basic matching rule.
 It matches a desktop entry if the desktop entry has the given desktop-file id
 ************************************************/
void XdgMenuLayoutProcessor::processFilenameTag(const QDomElement &element)
{
    QString id = element.text();

    QDomElement appLink = searchElement("AppLink"_L1, "id"_L1, id);
    if (!appLink.isNull())
        mResult.appendChild(appLink);
}


/************************************************
 Its contents references an immediate sub-menu of the current menu, as such it should never contain
 a slash. If no such sub-menu exists the element should be ignored.
 This element may have various attributes, the default values are taken from the DefaultLayout key.

 show_empty [ bool ]
    defines whether a menu that contains no desktop entries and no sub-menus
    should be shown at all.

 inline [ bool ]
    If the inline attribute is "true" the menu that is referenced may be copied into the current
    menu at the current point instead of being inserted as sub-menu of the current menu.

 inline_limit [ int ]
    defines the maximum number of entries that can be inlined. If the sub-menu has more entries
    than inline_limit, the sub-menu will not be inlined. If the inline_limit is 0 (zero) there
    is no limit.

 inline_header [ bool ]
    defines whether an inlined menu should be preceded with a header entry listing the caption of
    the sub-menu.

 inline_alias [ bool ]
    defines whether a single inlined entry should adopt the caption of the inlined menu. In such
    case no additional header entry will be added regardless of the value of the inline_header
    attribute.

 Example: if a menu has a sub-menu titled "WordProcessor" with a single entry "OpenOffice 4.2", and
 both inline="true" and inline_alias="true" are specified then this would result in the
 "OpenOffice 4.2" entry being inlined in the current menu but the "OpenOffice 4.2" caption of the
 entry would be replaced with "WordProcessor".
 ************************************************/
void XdgMenuLayoutProcessor::processMenunameTag(const QDomElement &element)
{
    QString id = element.text();
    QDomElement menu = searchElement("Menu"_L1, "name"_L1, id);
    if (menu.isNull())
        return;

    LayoutParams params = mDefaultParams;
    setParams(element, &params);

    int count = childsCount(menu);

    if (count == 0)
    {
        if (params.mShowEmpty)
        {
            menu.setAttribute("keep"_L1, "true"_L1);
            mResult.appendChild(menu);
        }
        return;
    }


    bool doInline = params.mInline &&
                    (!params.mInlineLimit || params.mInlineLimit > count);

    bool doAlias = params.mInlineAlias &&
                   doInline && (count == 1);

    bool doHeader = params.mInlineHeader &&
                    doInline && !doAlias;


    if (!doInline)
    {
        mResult.appendChild(menu);
        return;
    }


    // Header ....................................
    if (doHeader)
    {
        QDomElement header = mLayout.ownerDocument().createElement("Header"_L1);

        QDomNamedNodeMap attrs = menu.attributes();
        for (int i=0; i < attrs.count(); ++i)
        {
            header.setAttributeNode(attrs.item(i).toAttr());
        }

        mResult.appendChild(header);
    }

    // Alias .....................................
    if (doAlias)
    {
        menu.firstChild().toElement().setAttribute("title"_L1, menu.attribute("title"_L1));
    }

    // Inline ....................................
    MutableDomElementIterator it(menu);
    while (it.hasNext())
    {
        mResult.appendChild(it.next());
    }

}


/************************************************
 It indicates a suggestion to draw a visual separator at this point in the menu.
 <Separator> elements at the start of a menu, at the end of a menu or that directly
 follow other <Separator> elements may be ignored.
 ************************************************/
void XdgMenuLayoutProcessor::processSeparatorTag(const QDomElement &element)
{
    QDomElement separator = element.ownerDocument().createElement("Separator"_L1);
    mResult.appendChild(separator);
}


/************************************************
 It indicates the point where desktop entries and sub-menus that are not explicitly mentioned
 within the <Layout> or <DefaultLayout> element are to be inserted.
 It has a type attribute that indicates which elements should be inserted:

 type="menus"
    means that all sub-menus that are not explicitly mentioned should be inserted in alphabetical
    order of their visual caption at this point.

 type="files" means that all desktop entries contained in this menu that are not explicitly
    mentioned should be inserted in alphabetical order of their visual caption at this point.

 type="all" means that a mix of all sub-menus and all desktop entries that are not explicitly
    mentioned should be inserted in alphabetical order of their visual caption at this point.

 ************************************************/
void XdgMenuLayoutProcessor::processMergeTag(const QDomElement &element)
{
    QString type = element.attribute("type"_L1);
    QList<QDomElement> elements;
    MutableDomElementIterator it(mElement);

    while (it.hasNext())
    {
        QDomElement e = it.next();
        if (
            ((type == "menus"_L1 || type == "all"_L1) && e.tagName() == "Menu"_L1) ||
            ((type == "files"_L1 || type == "all"_L1) && e.tagName() == "AppLink"_L1)
           )
            elements.append(e);
    }

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(elements.begin(), elements.end(),
              [&](const QDomElement &a, const QDomElement &b) {
                  return collator.compare(a.attribute("title"_L1),
                                          b.attribute("title"_L1)) < 0;
              });

    for (const QDomElement &e : elements) {
        mResult.insertBefore(e, element);
    }

    mResult.removeChild(element);
}
