/* $Id: UIMonitorCommon.h $ */
/** @file
 * VBox Qt GUI - UIMonitorCommon class declaration.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_activity_UIMonitorCommon_h
#define FEQT_INCLUDED_SRC_activity_UIMonitorCommon_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** UIDebuggerMetricData is used as data storage while parsing the xml stream received from IMachineDebugger. */
struct UIDebuggerMetricData
{
    UIDebuggerMetricData()
        : m_counter(0){}
    UIDebuggerMetricData(const QString &strName, quint64 counter)
        : m_strName(strName)
        , m_counter(counter){}
    QString m_strName;
    quint64 m_counter;
};


class SHARED_LIBRARY_STUFF UIMonitorCommon
{

public:

    /** @name Static utility methods that query and parse IMachineDebugger outputs for specific metrix types.
      * @{ */
        static void getNetworkLoad(CMachineDebugger &debugger, quint64 &uOutNetworkReceived, quint64 &uOutNetworkTransmitted);
        static void getDiskLoad(CMachineDebugger &debugger, quint64 &uOutDiskWritten, quint64 &uOutDiskRead);
        static void getVMMExitCount(CMachineDebugger &debugger, quint64 &uOutVMMExitCount);
    /** @} */
        static void getRAMLoad(CPerformanceCollector &comPerformanceCollector, QVector<QString> &nameList,
                               QVector<CUnknown>& objectList, quint64 &iOutTotalRAM, quint64 &iOutFreeRAM);


        static QPainterPath doughnutSlice(const QRectF &outerRectangle, const QRectF &innerRectangle, float fStartAngle, float fSweepAngle);
        static QPainterPath wholeArc(const QRectF &rectangle);
        static void drawCombinedDoughnutChart(quint64 data1, const QColor &data1Color,
                                              quint64 data2, const QColor &data2Color,
                                              QPainter &painter, quint64  iMaximum,
                                              const QRectF &chartRect, const QRectF &innerRect, int iOverlayAlpha);

        /* Returns a rectangle which is co-centric with @p outerFrame and scaled by @p fScaleX and fScaleY. */
        static QRectF getScaledRect(const QRectF &outerFrame, float fScaleX, float fScaleY);

        static void drawDoughnutChart(QPainter &painter, quint64 iMaximum, quint64 data,
                                      const QRectF &chartRect, const QRectF &innerRect, int iOverlayAlpha, const QColor &color);

private:

    /** Parses the xml string we get from the IMachineDebugger and returns an array of UIDebuggerMetricData. */
    static QVector<UIDebuggerMetricData> getAndParseStatsFromDebugger(CMachineDebugger &debugger, const QString &strQuery);

};

#endif /* !FEQT_INCLUDED_SRC_activity_UIMonitorCommon_h */
