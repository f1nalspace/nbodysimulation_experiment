#ifndef CHART_H
#define CHART_H

#include <math.h>
#include <vector>
#include <string>
#include <algorithm>

#include "vecmath.h"
#include "render.h"
#include "utils.h"
#include "font.h"

static inline double GetChartNumber(double range, bool roundIt) {
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
		range = GetChartNumber(inputMax - inputMin, false);
		tickSpacing = GetChartNumber(range / (maxTicks - 1.0), true);
		min = floor(inputMin / tickSpacing) * tickSpacing;
		max = ceil(inputMax / tickSpacing) * tickSpacing;
	}

	float MapValueToPosition(const double value, const float maxPos) const {
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

	void RenderBars(Render::CommandBuffer *commandBuffer, const float viewportLBWH[4], Font *font, Render::TextureHandle fontTexture, const float fontHeight) {
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
		std::string maxAxisLabel = StringFormat(axisFormat.c_str(), yAxis.max);
		float maxAxisTextWidth = GetTextWidth(maxAxisLabel.c_str(), (uint32_t)maxAxisLabel.size(), font, tickLabelFontHeight);
		float yAxisWidth = maxAxisTextWidth + axisMargin;

		float chartWidth = areaWidth - yAxisWidth;
		float chartOriginX = areaLeft + yAxisWidth;
		float chartOriginY = areaBottom + sampleAxisHeight + legendHeight;

		float sampleWidth = chartWidth / (float)sampleCount;
		float sampleMargin = 10;
		float subSampleMargin = 5;

		// Chart area
		Render::PushRectangle(commandBuffer, Vec2f(areaLeft, areaBottom), Vec2f(areaWidth, areaHeight), Vec4f(0.1f, 0.1f, 0.1f, 1.0f), true);

		// Grid
		Vec4f gridXLineColor = Vec4f(0.25f, 0.25f, 0.25f, 1);
		Vec4f gridYLineColor = Vec4f(0.25f, 0.25f, 0.25f, 1);
		for (int sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
			Render::PushLine(commandBuffer, Vec2f(chartOriginX + (float)sampleIndex * sampleWidth, chartOriginY), Vec2f(chartOriginX + (float)sampleIndex * sampleWidth, chartOriginY + chartHeight), gridXLineColor, 1.0f);
		}

		for (int tickIndex = 0; tickIndex <= tickCount; ++tickIndex) {
			double tickValue = yAxis.min + yAxis.tickSpacing * (double)tickIndex;
			float tickHeight = yAxis.MapValueToPosition(tickValue, chartHeight);
			Render::PushLine(commandBuffer, Vec2f(chartOriginX, chartOriginY + tickHeight), Vec2f(chartOriginX + chartWidth, chartOriginY + tickHeight), gridYLineColor, 1.0f);
		}

		// Axis lines
		float axisLineExtend = 10.0f;
		Vec4f axisLineColor = Vec4f(0.65f, 0.65f, 0.65f, 1);
		Render::PushLine(commandBuffer, Vec2f(chartOriginX - axisLineExtend, chartOriginY), Vec2f(chartOriginX + chartWidth, chartOriginY), axisLineColor, 1.0f);
		Render::PushLine(commandBuffer, Vec2f(chartOriginX, chartOriginY - axisLineExtend), Vec2f(chartOriginX, chartOriginY + chartHeight), axisLineColor, 1.0f);

		// Tick marks
		Vec4f tickMarkLineColor = Vec4f(0.2f, 0.2f, 0.2f, 1);
		for (int tickIndex = 0; tickIndex <= tickCount; ++tickIndex) {
			double tickValue = yAxis.min + yAxis.tickSpacing * (double)tickIndex;
			float tickHeight = yAxis.MapValueToPosition(tickValue, chartHeight);
			Render::PushLine(commandBuffer, Vec2f(chartOriginX, chartOriginY + tickHeight), Vec2f(chartOriginX - axisMargin, chartOriginY + tickHeight), tickMarkLineColor, 1.0f);
		}

		// Tick labels
		Vec4f tickLabelColor = Vec4f(1.0f, 1.0f, 1.0f, 1);
		for (int tickIndex = 0; tickIndex <= tickCount; ++tickIndex) {
			double tickValue = yAxis.min + yAxis.tickSpacing * (double)tickIndex;
			float tickHeight = yAxis.MapValueToPosition(tickValue, chartHeight);
			std::string tickLabel = StringFormat(axisFormat.c_str(), tickValue);
			float tickLabelWidth = GetTextWidth(tickLabel.c_str(), (uint32_t)tickLabel.size(), font, tickLabelFontHeight);
			float tickY = chartOriginY + tickHeight - tickLabelFontHeight * 0.5f;
			float tickX = chartOriginX - axisMargin - tickLabelWidth;
			Render::PushText(commandBuffer, Vec2f(tickX, tickY), tickLabel.c_str(), font, fontTexture, tickLabelFontHeight, tickLabelColor);
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
				Vec2f rectPos = Vec2f(sampleLeft, sampleBottom);
				Vec2f rectSize = Vec2f(abs(sampleRight - sampleLeft), abs(sampleBottom - sampleTop));
				Render::PushRectangle(commandBuffer, rectPos, rectSize, seriesColor, true);
			}
		}

		// Sample labels
		for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
			const std::string &label = sampleLabels[sampleIndex];
			const char *text = label.c_str();
			size_t textLen = label.size();
			float textWidth = GetTextWidth(text, (uint32_t)textLen, font, sampleLabelFontHeight);
			float xLeft = chartOriginX + (float)sampleIndex * sampleWidth + sampleWidth * 0.5f - textWidth * 0.5f;
			float yMiddle = chartOriginY - sampleLabelFontHeight - sampleAxisMargin;
			Render::PushText(commandBuffer, Vec2f(xLeft, yMiddle), text, font, fontTexture, sampleLabelFontHeight, Vec4f(1, 1, 1, 1));
		}

		// Legend
		float legendCurLeft = areaLeft;
		float legendBottom = areaBottom + legendMargin;
		for (int seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
			ChartSeries *series = &seriesItems[seriesIndex];
			Vec4f legendColor = series->color;

			Render::PushRectangle(commandBuffer, Vec2f(legendCurLeft, legendBottom), Vec2f(legendBulletSize, legendBulletSize), legendColor, true);
			legendCurLeft += legendBulletSize + legendBulletPadding;

			const char *legendLabel = series->title.c_str();
			float labelWidth = (float)GetTextWidth(legendLabel, (uint32_t)strlen(legendLabel), font, legendFontHeight);
			float labelY = legendBottom - legendFontHeight * 0.5f + legendBulletSize * 0.5f;
			Render::PushText(commandBuffer, Vec2f(legendCurLeft, labelY), legendLabel, font, fontTexture, legendFontHeight, Vec4f(1, 1, 1, 1));
			legendCurLeft += labelWidth + legendLabelPadding;
		}
	}
};

#endif
