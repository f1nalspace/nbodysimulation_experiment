#ifndef CHART_H
#define CHART_H

#include <math.h>
#include <vector>
#include <string>
#include <algorithm>
#include <GL\glew.h>

#include "vecmath.h"
#include "render.h"
#include "utils.h"

double GetNiceNumber(double range, bool roundIt) {
	double exponent = floor(log10(range));
	double fraction = range / pow(10.0, exponent);
	double resultingFraction;
	if (roundIt) {
		if (fraction < 1.5) {
			resultingFraction = 1.0;
		} else if (fraction < 3.0) {
			resultingFraction = 2.0;
		} else if (fraction < 7.0) {
			resultingFraction = 5.0;
		} else {
			resultingFraction = 10.0;
		}
	} else {
		if (fraction <= 1.0) {
			resultingFraction = 1.0;
		} else if (fraction <= 2.0) {
			resultingFraction = 2.0;
		} else if (fraction <= 5.0) {
			resultingFraction = 5.0;
		} else {
			resultingFraction = 10.0;
		}
	}
	double result = resultingFraction * pow(10.0, exponent);
	return(result);
}

struct ChartAxis {
	double range;
	double tickSpacing;
	double min;
	double max;

	ChartAxis(double inputMin, double inputMax, double maxTicks) {
		range = GetNiceNumber(inputMax - inputMin, false);
		tickSpacing = GetNiceNumber(range / (maxTicks - 1.0), true);
		min = floor(inputMin / tickSpacing) * tickSpacing;
		max = ceil(inputMax / tickSpacing) * tickSpacing;
	}

	float MapValueToPosition(const double value, const float maxPos) {
		float factor = maxPos / (float)range;
		float result = (float)value * factor;
		return(result);
	}
};

struct ChartSeries {
	std::string title;
	std::vector<double> values;
	Vec4f color;

	void AddValue(const double value) {
		values.push_back(value);
	}
};

struct Chart {
	std::vector<ChartSeries> seriesItems;
	std::vector<std::string> sampleLabels;

	std::string axisFormat;

	Chart() {
		axisFormat = "%.2f";
	}

	inline void AddSeries(const ChartSeries &series) {
		this->seriesItems.push_back(series);
	}

	inline void AddSampleLabel(const std::string &sampleLabel) {
		this->sampleLabels.push_back(sampleLabel);
	}

	void RenderBars(const float viewportLBWH[4], void *font, const float fontHeight) {
		float viewportLeft = viewportLBWH[0];
		float viewportBottom = viewportLBWH[1];
		float viewportWidth = viewportLBWH[2];
		float viewportHeight = viewportLBWH[3];

		float areaWidth = viewportWidth;
		float areaHeight = viewportHeight;
		float areaLeft = viewportLeft;
		float areaBottom = viewportBottom;

		float sampleLabelFontHeight = (float)fontHeight;
		float sampleAxisMargin = 10.0f;
		float sampleAxisHeight = sampleLabelFontHeight + (sampleAxisMargin * 2.0f);

		float legendLabelPadding = 5.0f;
		float legendBulletPadding = 5.0f;
		float legendMargin = 0.0f;
		float legendFontHeight = (float)fontHeight;
		float legendBulletSize = (float)fontHeight * 0.75f;
		float legendHeight = std::max(legendFontHeight, legendBulletSize) + (legendMargin * 2.0f);

		float tickLabelFontHeight = (float)fontHeight;

		double originalMinValue = 0.0;
		double originalMaxValue = 0.0;
		size_t sampleCount = 0;
		size_t seriesCount = seriesItems.size();
		for (int seriesIndex = 0; seriesIndex < seriesItems.size(); ++seriesIndex) {
			ChartSeries *series = &seriesItems[seriesIndex];
			sampleCount = std::max(sampleCount, series->values.size());
			for (int valueIndex = 0; valueIndex < series->values.size(); ++valueIndex) {
				double value = series->values[valueIndex];
				originalMinValue = std::min(originalMinValue, value);
				originalMaxValue = std::max(originalMaxValue, value);
			}
		}

		float chartHeight = areaHeight - (sampleAxisHeight + legendHeight + tickLabelFontHeight * 0.5f);

		int maxTickCount = std::max((int)(chartHeight / tickLabelFontHeight), 2);
		ChartAxis yAxis = ChartAxis(originalMinValue, originalMaxValue, (double)maxTickCount);
		size_t tickCount = (size_t)(yAxis.range / yAxis.tickSpacing);

		float axisMargin = 10;
		std::string maxAxisLabel = StringFormat(axisFormat, (size_t)256, yAxis.max);
		float yAxisWidth = GetTextWidth(maxAxisLabel.c_str(), font) + axisMargin;

		float chartWidth = areaWidth - yAxisWidth;
		float chartOriginX = areaLeft + yAxisWidth;
		float chartOriginY = areaBottom + sampleAxisHeight + legendHeight;

		float sampleWidth = chartWidth / (float)sampleCount;
		float sampleMargin = 10;
		float subSampleMargin = 5;

		// Chart area
		glColor4f(0.1f, 0.1f, 0.1f, 1);
		glBegin(GL_QUADS);
		glVertex2f(areaLeft, areaBottom);
		glVertex2f(areaLeft + areaWidth, areaBottom);
		glVertex2f(areaLeft + areaWidth, areaBottom + areaHeight);
		glVertex2f(areaLeft, areaBottom + areaHeight);
		glEnd();

		// Grid
		Vec4f gridXLineColor = Vec4f(0.25f, 0.25f, 0.25f, 1);
		Vec4f gridYLineColor = Vec4f(0.25f, 0.25f, 0.25f, 1);

		glColor4fv(&gridXLineColor.m[0]);
		glLineWidth(1);
		glBegin(GL_LINES);
		for (int sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
			glVertex2f(chartOriginX + (float)sampleIndex * sampleWidth, chartOriginY);
			glVertex2f(chartOriginX + (float)sampleIndex * sampleWidth, chartOriginY + chartHeight);
		}
		glEnd();
		glLineWidth(1);

		glColor4fv(&gridYLineColor.m[0]);
		glLineWidth(1);
		glBegin(GL_LINES);
		for (int tickIndex = 0; tickIndex <= tickCount; ++tickIndex) {
			double tickValue = yAxis.min + yAxis.tickSpacing * (double)tickIndex;
			float tickHeight = yAxis.MapValueToPosition(tickValue, chartHeight);
			glVertex2f(chartOriginX, chartOriginY + tickHeight);
			glVertex2f(chartOriginX + chartWidth, chartOriginY + tickHeight);
		}
		glEnd();
		glLineWidth(1);

		// Axis lines
		float axisLineExtend = 10.0f;
		Vec4f axisLineColor = Vec4f(0.65f, 0.65f, 0.65f, 1);
		glColor4fv(&axisLineColor.m[0]);
		glLineWidth(1);
		glBegin(GL_LINES);
		glVertex2f(chartOriginX - axisLineExtend, chartOriginY);
		glVertex2f(chartOriginX + chartWidth, chartOriginY);
		glVertex2f(chartOriginX, chartOriginY - axisLineExtend);
		glVertex2f(chartOriginX, chartOriginY + chartHeight);
		glEnd();
		glLineWidth(1);

		// Tick marks
		Vec4f tickMarkLineColor = Vec4f(0.2f, 0.2f, 0.2f, 1);
		glColor4fv(&tickMarkLineColor.m[0]);
		glLineWidth(1);
		glBegin(GL_LINES);
		for (int tickIndex = 0; tickIndex <= tickCount; ++tickIndex) {
			double tickValue = yAxis.min + yAxis.tickSpacing * (double)tickIndex;
			float tickHeight = yAxis.MapValueToPosition(tickValue, chartHeight);
			glVertex2f(chartOriginX, chartOriginY + tickHeight);
			glVertex2f(chartOriginX - axisMargin, chartOriginY + tickHeight);
		}
		glEnd();
		glLineWidth(1);

		// Tick labels
		Vec4f tickLabelColor = Vec4f(1.0f, 1.0f, 1.0f, 1);
		for (int tickIndex = 0; tickIndex <= tickCount; ++tickIndex) {
			double tickValue = yAxis.min + yAxis.tickSpacing * (double)tickIndex;
			float tickHeight = yAxis.MapValueToPosition(tickValue, chartHeight);
			std::string tickLabel = StringFormat(axisFormat, 256, tickValue);
			float tickLabelWidth = (float)GetTextWidth(tickLabel.c_str(), font);
			float tickY = chartOriginY + tickHeight - tickLabelFontHeight * 0.5f;
			float tickX = chartOriginX - axisMargin - tickLabelWidth;
			RenderText(tickX, tickY, tickLabel.c_str(), tickLabelColor, font);
		}

		// Bars
		float barWidth = sampleWidth - (sampleMargin * 2.0f);
		float seriesBarWidth = (barWidth - (subSampleMargin * (float)(seriesCount - 1))) / (float)seriesCount;
		for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
			for (int seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
				ChartSeries *series = &seriesItems[seriesIndex];
				Vec4f seriesColor = series->color;
				double value = series->values[sampleIndex];
				float sampleHeight = yAxis.MapValueToPosition(value, chartHeight);
				float sampleLeft = chartOriginX + (float)sampleIndex * sampleWidth + sampleMargin + ((float)seriesIndex * seriesBarWidth) + ((float)seriesIndex * subSampleMargin);
				float sampleRight = sampleLeft + seriesBarWidth;
				float sampleBottom = chartOriginY;
				float sampleTop = chartOriginY + sampleHeight;
				FillRectangle(Vec2f(sampleLeft, sampleBottom), Vec2f(abs(sampleRight - sampleLeft), abs(sampleBottom - sampleTop)), seriesColor);
			}
		}

		// Sample labels
		for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
			const char *sampleLabel = sampleLabels[sampleIndex].c_str();
			float textWidth = (float)GetTextWidth(sampleLabel, font);
			float xLeft = chartOriginX + (float)sampleIndex * sampleWidth + sampleWidth * 0.5f - textWidth * 0.5f;
			float yMiddle = chartOriginY - sampleLabelFontHeight - sampleAxisMargin;
			RenderText(xLeft, yMiddle, sampleLabel, Vec4f(1, 1, 1, 1), font);
		}

		// Legend
		float legendCurLeft = areaLeft;
		float legendBottom = areaBottom + legendMargin;
		for (int seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
			ChartSeries *series = &seriesItems[seriesIndex];
			Vec4f legendColor = series->color;

			FillRectangle(Vec2f(legendCurLeft, legendBottom), Vec2f(legendBulletSize, legendBulletSize), legendColor);
			legendCurLeft += legendBulletSize + legendBulletPadding;

			const char *legendLabel = series->title.c_str();
			float labelWidth = (float)GetTextWidth(legendLabel, font);
			float labelY = legendBottom - legendFontHeight * 0.5f + legendBulletSize * 0.5f;
			RenderText(legendCurLeft, labelY, legendLabel, Vec4f(1, 1, 1, 1), font);
			legendCurLeft += labelWidth + legendLabelPadding;
		}
	}
};

#endif
