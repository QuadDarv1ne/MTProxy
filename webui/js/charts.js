/**
 * MTProxy Charts Module
 * Simple chart rendering without external dependencies
 */

class ChartsManager {
    constructor() {
        this.colors = {
            primary: '#3b82f6',
            success: '#10b981',
            warning: '#f59e0b',
            danger: '#ef4444',
            info: '#06b6d4',
        };
    }

    /**
     * Render a simple bar chart
     */
    renderBarChart(containerId, data, options = {}) {
        const container = document.getElementById(containerId);
        if (!container || !data || data.length === 0) return;

        const {
            labels = [],
            values = [],
            colors = [this.colors.primary],
            height = 200,
        } = options;

        const maxValue = Math.max(...values, 1);
        
        const bars = values.map((value, index) => {
            const barHeight = (value / maxValue) * (height - 40);
            const color = colors[index % colors.length];
            const label = labels[index] || '';
            
            return `
                <div class="bar-chart-item">
                    <div class="bar-chart-bar" style="height: ${barHeight}px; background: ${color}">
                        <span class="bar-chart-value">${this.formatValue(value)}</span>
                    </div>
                    <span class="bar-chart-label">${label}</span>
                </div>
            `;
        }).join('');

        container.innerHTML = `
            <div class="bar-chart" style="height: ${height}px">
                ${bars}
            </div>
        `;
    }

    /**
     * Render a line chart (sparkline style)
     */
    renderLineChart(containerId, data, options = {}) {
        const container = document.getElementById(containerId);
        if (!container || !data || data.length === 0) return;

        const {
            labels = [],
            values = [],
            color = this.colors.primary,
            height = 200,
            showArea = true,
        } = options;

        const maxValue = Math.max(...values, 1);
        const minValue = Math.min(...values, 0);
        const range = maxValue - minValue || 1;
        
        const width = container.clientWidth || 400;
        const pointSpacing = width / (data.length - 1 || 1);
        
        // Generate SVG path
        const points = values.map((value, index) => {
            const x = index * pointSpacing;
            const y = height - ((value - minValue) / range) * (height - 40) - 20;
            return `${x},${y}`;
        }).join(' ');

        // Generate area path
        const areaPath = showArea 
            ? `M0,${height} L${points.replace(/ /g, ' L')} L${width},${height} Z`
            : '';

        container.innerHTML = `
            <div class="line-chart" style="height: ${height}px">
                <svg width="100%" height="100%" viewBox="0 0 ${width} ${height}" preserveAspectRatio="none">
                    ${showArea ? `<defs>
                        <linearGradient id="gradient-${containerId}" x1="0%" y1="0%" x2="0%" y2="100%">
                            <stop offset="0%" style="stop-color:${color};stop-opacity:0.3" />
                            <stop offset="100%" style="stop-color:${color};stop-opacity:0" />
                        </linearGradient>
                    </defs>
                    <path d="${areaPath}" fill="url(#gradient-${containerId})" />` : ''}
                    <polyline 
                        points="${points}" 
                        fill="none" 
                        stroke="${color}" 
                        stroke-width="2"
                        vector-effect="non-scaling-stroke"
                    />
                    ${values.map((value, index) => {
                        const x = index * pointSpacing;
                        const y = height - ((value - minValue) / range) * (height - 40) - 20;
                        return `<circle cx="${x}" cy="${y}" r="3" fill="${color}" />`;
                    }).join('')}
                </svg>
                ${labels.length > 0 ? `
                    <div class="line-chart-labels">
                        <span>${labels[0] || ''}</span>
                        <span>${labels[labels.length - 1] || ''}</span>
                    </div>
                ` : ''}
            </div>
        `;
    }

    /**
     * Render a gauge/progress chart
     */
    renderGauge(containerId, value, options = {}) {
        const container = document.getElementById(containerId);
        if (!container) return;

        const {
            min = 0,
            max = 100,
            label = '',
            color = this.getColorForValue(value, max),
            size = 150,
            showValue = true,
        } = options;

        const percentage = ((value - min) / (max - min)) * 100;
        const clampedPercentage = Math.min(Math.max(percentage, 0), 100);
        
        const circumference = 2 * Math.PI * (size / 2 - 10);
        const offset = circumference - (clampedPercentage / 100) * circumference;

        container.innerHTML = `
            <div class="gauge-chart" style="width: ${size}px; height: ${size}px">
                <svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">
                    <circle
                        cx="${size / 2}"
                        cy="${size / 2}"
                        r="${size / 2 - 10}"
                        fill="none"
                        stroke="var(--bg-secondary)"
                        stroke-width="10"
                    />
                    <circle
                        cx="${size / 2}"
                        cy="${size / 2}"
                        r="${size / 2 - 10}"
                        fill="none"
                        stroke="${color}"
                        stroke-width="10"
                        stroke-dasharray="${circumference}"
                        stroke-dashoffset="${offset}"
                        stroke-linecap="round"
                        transform="rotate(-90 ${size / 2} ${size / 2})"
                    />
                    ${showValue ? `
                        <text
                            x="${size / 2}"
                            y="${size / 2}"
                            text-anchor="middle"
                            dominant-baseline="middle"
                            class="gauge-value"
                        >
                            ${this.formatValue(value)}
                        </text>
                    ` : ''}
                </svg>
                ${label ? `<div class="gauge-label">${label}</div>` : ''}
            </div>
        `;
    }

    /**
     * Render a donut chart
     */
    renderDonutChart(containerId, data, options = {}) {
        const container = document.getElementById(containerId);
        if (!container || !data || data.length === 0) return;

        const {
            labels = [],
            colors = [this.colors.primary, this.colors.success, this.colors.warning, this.colors.danger],
            size = 200,
        } = options;

        const total = data.reduce((sum, value) => sum + value, 0);
        let cumulativePercentage = 0;

        const segments = data.map((value, index) => {
            const percentage = (value / total) * 100;
            const startPercentage = cumulativePercentage;
            cumulativePercentage += percentage;

            const startAngle = (startPercentage / 100) * 360;
            const endAngle = (cumulativePercentage / 100) * 360;
            
            const color = colors[index % colors.length];
            const label = labels[index] || '';

            return {
                startAngle,
                endAngle,
                color,
                label,
                value,
                percentage,
            };
        });

        // Create SVG path for segment
        const getSegmentPath = (startAngle, endAngle, innerRadius, outerRadius) => {
            const start1 = this.polarToCartesian(size / 2, size / 2, outerRadius, startAngle);
            const end1 = this.polarToCartesian(size / 2, size / 2, outerRadius, endAngle);
            const start2 = this.polarToCartesian(size / 2, size / 2, innerRadius, startAngle);
            const end2 = this.polarToCartesian(size / 2, size / 2, innerRadius, endAngle);

            const largeArcFlag = endAngle - startAngle <= 180 ? 0 : 1;

            return [
                `M ${start1.x} ${start1.y}`,
                `A ${outerRadius} ${outerRadius} 0 ${largeArcFlag} 1 ${end1.x} ${end1.y}`,
                `L ${end2.x} ${end2.y}`,
                `A ${innerRadius} ${innerRadius} 0 ${largeArcFlag} 0 ${start2.x} ${start2.y}`,
                'Z',
            ].join(' ');
        };

        const innerRadius = size / 2 - 50;
        const outerRadius = size / 2 - 10;

        const segmentsSvg = segments.map(seg => 
            `<path d="${getSegmentPath(seg.startAngle, seg.endAngle, innerRadius, outerRadius)}" 
                   fill="${seg.color}" 
                   stroke="var(--bg-card)" 
                   stroke-width="2"
                   class="donut-segment"
                   data-label="${seg.label}"
                   data-value="${seg.value}"
                   data-percentage="${seg.percentage.toFixed(1)}%"
            />`
        ).join('');

        const legend = segments.map((seg, index) => `
            <div class="legend-item">
                <span class="legend-color" style="background: ${seg.color}"></span>
                <span>${seg.label}</span>
                <span class="legend-value">${seg.percentage.toFixed(1)}%</span>
            </div>
        `).join('');

        container.innerHTML = `
            <div class="donut-chart-container">
                <div class="donut-chart" style="width: ${size}px; height: ${size}px">
                    <svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">
                        ${segmentsSvg}
                    </svg>
                </div>
                <div class="donut-legend">
                    ${legend}
                </div>
            </div>
        `;
    }

    // Helper methods
    polarToCartesian(cx, cy, r, angle) {
        const rad = (angle - 90) * Math.PI / 180;
        return {
            x: cx + r * Math.cos(rad),
            y: cy + r * Math.sin(rad),
        };
    }

    getColorForValue(value, max) {
        const percentage = (value / max) * 100;
        if (percentage < 50) return this.colors.success;
        if (percentage < 75) return this.colors.warning;
        return this.colors.danger;
    }

    formatValue(value) {
        if (value >= 1e9) return (value / 1e9).toFixed(1) + 'B';
        if (value >= 1e6) return (value / 1e6).toFixed(1) + 'M';
        if (value >= 1e3) return (value / 1e3).toFixed(1) + 'K';
        return value.toString();
    }
}

// Export singleton
window.chartsManager = new ChartsManager();
