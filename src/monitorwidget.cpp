/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     AaronZhang <ya.zhang@archermind.com>
 *
 * Maintainer: AaronZhang <ya.zhang@archermind.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "monitorwidget.h"
#include "deviceinfoparser.h"
#include "math.h"
#include <QDate>
#include <DApplication>

DWIDGET_USE_NAMESPACE

int gcd(int a,int b){
    if(a<b)
        std::swap(a,b);
    if(a%b==0)
        return b;
    else
        return gcd(b,a%b);
}

bool findAspectRatio(int width, int height, int& ar_w, int& ar_h)
{
    float r1 = float(width)/float(height);

    for(ar_w = 21; ar_w > 0; --ar_w)
    {
        for(ar_h = 21; ar_h > 0; --ar_h)
        {
            if( std::abs(r1 - float(ar_w)/float(ar_h))/r1 < 0.01)
            {
                int r = gcd(ar_w, ar_h);
                ar_w/= r;
                ar_h/= r;
                return true;
            }
        }
    }

    return false;
}

MonitorWidget::MonitorWidget(QWidget *parent) : DeviceInfoWidgetBase(parent, DApplication::translate("Main", "Monitor"))
{
    initWidget();
}

void MonitorWidget::initWidget()
{
    //setTitle(DApplication::translate("Main", "Monitor")  + DApplication::translate("Main", " Info"));

    QList<QStringList> tabList;
    QList<ArticleStruct> articles;
    QSet<QString> existArticles;

    QStringList hwinfMonitorList = DeviceInfoParserInstance.getHwinfoMonitorList();
    QStringList xrandrMonitorList = DeviceInfoParserInstance.getXrandrMonitorList();

    for(int i = 0; i < std::max(hwinfMonitorList.size(), xrandrMonitorList.size()); ++i)
    {
        articles.clear();
        existArticles.clear();

        ArticleStruct name("Name");
        ArticleStruct vendor("Vendor");
        ArticleStruct currentResolution("Resolution");
        ArticleStruct resolutionList("Support Resolution");
        ArticleStruct displayRete("Display Rate");

        ArticleStruct monitorSize(DApplication::translate("Monitor", "Size"));

        double inch = 0.0;

        if( i < hwinfMonitorList.size() )
        {
            const QString& monitor = hwinfMonitorList.at(i);

            name.queryData("hwinfo", monitor, "Model");
            name.value = name.value.remove("\"");

            existArticles.insert("Model");

            vendor.queryData("hwinfo", monitor, "Vendor");
            QString abb;
            QRegExp rx("(^[\\s\\S]*)\"([\\s\\S]+)\"$");
            if( rx.exactMatch(vendor.value) )
            {
                abb = rx.cap(1).trimmed();
                vendor.value = rx.cap(2).trimmed();
            }
            existArticles.insert("Vendor");

            name.value.remove(abb);

            if(name.value.contains(vendor.value) == false)
            {
                name.value = vendor.value + " " + name.value;
            }

            articles.push_back(name);
            articles.push_back(vendor);

            ArticleStruct serial("Serial Number");
            serial.queryData("hwinfo", monitor, "Serial ID");
            serial.value.remove("\"");
            articles.push_back(serial);
            existArticles.insert("Serial ID");

            QString size = DeviceInfoParserInstance.queryData("hwinfo", monitor, "Size");

            monitorSize.value = parseMonitorSize(size, inch);

            articles.push_back(monitorSize);
            existArticles.insert("Size");

            ArticleStruct mDate("Manufacture Date");
            QString my = DeviceInfoParserInstance.queryData("hwinfo", monitor, "Year of Manufacture");
            if( my.isEmpty() == false && my != DApplication::translate("Main", "Unknown") )
            {
                mDate.value = my + DApplication::translate("Main", "Year");
            }

            QString mw = DeviceInfoParserInstance.queryData("hwinfo", monitor, "Week of Manufacture");
            if( mw.isEmpty() == false && mw != DApplication::translate("Main", "Unknown") && mw != "0")
            {
                mDate.value += " ";
                mDate.value += mw;
                mDate.value += DApplication::translate("Main", "Week");
            }
            articles.push_back(mDate);

            existArticles.insert("Year of Manufacture");
            existArticles.insert("Week of Manufacture");

            currentResolution.queryData("hwinfo", monitor, "Current Resolution");
            currentResolution.queryData("hwinfo", monitor, "Support Resolution");

            displayRete.value = parseDisplayRate(currentResolution.value);

            ArticleStruct frequencies("Frequencies");
            frequencies.queryData("hwinfo", monitor, "Frequencies");
            if(frequencies.isValid())
            {
                frequencies.value = frequencies.value.trimmed().split(",").last().trimmed();
                if(frequencies.isValid())
                {
                    currentResolution.value += " @";
                    currentResolution.value += frequencies.value;
                }
            }

            articles.push_back(currentResolution);
            existArticles.insert("Current Resolution");

            articles.push_back(displayRete);

            resolutionList.queryData("hwinfo", monitor, "Support Resolution");
        }

        ArticleStruct primaryMonitor("Primary Monitor");
        primaryMonitor.value = "No";
        if( i < xrandrMonitorList.size())
        {
            if( true == xrandrMonitorList.at(i).contains("primary", Qt::CaseInsensitive) )
            {
                primaryMonitor.value = "Yes";
            }

            //currentResolution.queryData("xrandr", xrandrMonitorList.at(i), "");
            if(currentResolution.isValid() == false)
            {
                currentResolution.queryData("xrandr", xrandrMonitorList.at(i), "Resolution");
                articles.push_back(currentResolution);
                displayRete.value = parseDisplayRate(currentResolution.value);
                articles.push_back(displayRete);
            }

            if(monitorSize.isValid() == false)
            {
                monitorSize.queryData("xrandr", xrandrMonitorList.at(i), "Size");
                monitorSize.value = parseMonitorSize(monitorSize.value, inch);
                articles.push_back(monitorSize);
            }
        }
        articles.push_back(primaryMonitor);

        ArticleStruct connectType("Connect Type");
        connectType.value = DApplication::translate("Main", "Unknown");
        if( i < xrandrMonitorList.size())
        {
            connectType.value = xrandrMonitorList.at(i);
            int index = connectType.value.indexOf(' ');
            if( index > 0 )
            {
                connectType.value = connectType.value.mid(0, index);
            }
            index = connectType.value.indexOf('-');
            if( index > 0 )
            {
                connectType.value = connectType.value.mid(0, index);
            }
        }
        articles.push_back(connectType);

        articles.push_back(resolutionList);

        addDevice( name.value, articles, hwinfMonitorList.size() );

        if( std::max(hwinfMonitorList.size(), xrandrMonitorList.size() ) > 1 )
        {
            QStringList tab =
            {
                name.value,
                vendor.value
            };

            tabList.push_back(tab);
        }

        if(overviewInfo_.value.isEmpty() == false)
        {
            overviewInfo_.value += " / ";
        }

        overviewInfo_.value += name.value;
        if(inch != 0.0)
        {
            if(overviewInfo_.isValid() == false)
            {
                overviewInfo_.value = QString::number(inch,10, 1) + " " + DApplication::translate("Main", "inch");
            }
            else
            {
                overviewInfo_.value += " (";
                overviewInfo_.value += QString::number(inch,10, 1) + " " + DApplication::translate("Main", "inch");
                overviewInfo_.value += ")";
            }
        }
    }

    if( hwinfMonitorList.size() > 1 )
    {
        QStringList headers = { "Name",  "Vendor" };
        addTable( headers, tabList);
    }
}

QString MonitorWidget::parseMonitorSize(const QString& size, double& inch)
{
    inch = 0.0;

    QString res = size;
    QRegExp re("^([\\d]*)x([\\d]*) mm$");
    if( re.exactMatch(size) )
    {
        double width = re.cap(1).toDouble()/2.54;
        double height = re.cap(2).toDouble()/2.54;
        inch = std::sqrt(width*width + height*height)/10.0;
        res = QString::number(inch,10, 1) + " " + DApplication::translate("Main", "inch") + " (";
        res += size;
        res += ")";
    }

    re.setPattern("([0-9]\\d*)mm x ([0-9]\\d*)mm");
    if( re.exactMatch(size) )
    {
        double width = re.cap(1).toDouble()/2.54;
        double height = re.cap(2).toDouble()/2.54;
        inch = std::sqrt(width*width + height*height)/10.0;
        res = QString::number(inch,10, 1) + " " + DApplication::translate("Main", "inch") + " (";
        res += size;
        res += ")";
    }

    return res;
}

QString MonitorWidget::parseDisplayRate(const QString& resulotion)
{
    QRegExp re("^([\\d]*)x([\\d]*)$");
    QString res;
    if( re.exactMatch(resulotion) )
    {
        int width = re.cap(1).toInt();
        int height = re.cap(2).toInt();
        int gys = gcd(width, height);
        int w = width/gys;
        int h = height/gys;

        if(w > 21)
        {
            findAspectRatio(w, h, w, h);
        }

        res = QString::number(w) + " : " + QString::number(h);
    }

    return res;
}
