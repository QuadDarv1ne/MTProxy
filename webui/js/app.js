/**
 * MTProxy Web UI Application
 */

class MTProxyApp {
    constructor() {
        this.api = window.mtproxyApi;
        this.refreshInterval = null;
        this.currentStats = null;
        this.statsHistory = [];
        this.maxHistoryPoints = 60;
        
        this.init();
    }

    init() {
        this.bindEvents();
        this.loadTheme();
        this.startAutoRefresh();
        this.loadDashboard();
    }

    bindEvents() {
        // Navigation
        document.querySelectorAll('.nav-link').forEach(link => {
            link.addEventListener('click', (e) => this.handleNavigation(e));
        });

        // Theme toggle
        document.getElementById('themeToggle').addEventListener('click', () => this.toggleTheme());

        // Refresh stats
        document.getElementById('refreshStats')?.addEventListener('click', () => this.loadDashboard());

        // Config form
        document.getElementById('saveConfig')?.addEventListener('click', () => this.saveConfig());
        document.getElementById('reloadConfig')?.addEventListener('click', () => this.loadConfig());

        // Secrets
        document.getElementById('addSecret')?.addEventListener('click', () => this.openModal('addSecretModal'));
        document.getElementById('addSecretForm')?.addEventListener('submit', (e) => this.handleAddSecret(e));

        // Modal close
        document.querySelectorAll('[data-modal]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const modalId = e.target.dataset.modal;
                if (modalId) this.closeModal(modalId);
            });
        });

        // Close modal on outside click
        document.querySelectorAll('.modal').forEach(modal => {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    this.closeModal(modal.id);
                }
            });
        });

        // Logs
        document.getElementById('refreshLogs')?.addEventListener('click', () => this.loadLogs());
        document.getElementById('logLevelFilter')?.addEventListener('change', () => this.loadLogs());
    }

    handleNavigation(e) {
        e.preventDefault();
        const page = e.currentTarget.dataset.page;
        
        // Update nav links
        document.querySelectorAll('.nav-link').forEach(link => {
            link.classList.remove('active');
        });
        e.currentTarget.classList.add('active');

        // Update pages
        document.querySelectorAll('.page').forEach(p => {
            p.classList.remove('active');
        });
        document.getElementById(`${page}-page`).classList.add('active');

        // Load page data
        if (page === 'config') {
            this.loadConfig();
        } else if (page === 'secrets') {
            this.loadSecrets();
        } else if (page === 'logs') {
            this.loadLogs();
        }
    }

    loadTheme() {
        const theme = localStorage.getItem('theme') || 'dark';
        document.documentElement.setAttribute('data-theme', theme);
    }

    toggleTheme() {
        const current = document.documentElement.getAttribute('data-theme');
        const next = current === 'dark' ? 'light' : 'dark';
        document.documentElement.setAttribute('data-theme', next);
        localStorage.setItem('theme', next);
    }

    startAutoRefresh() {
        this.refreshInterval = setInterval(() => {
            if (document.getElementById('dashboard-page').classList.contains('active')) {
                this.loadDashboard();
            }
        }, 5000); // Refresh every 5 seconds
    }

    stopAutoRefresh() {
        if (this.refreshInterval) {
            clearInterval(this.refreshInterval);
            this.refreshInterval = null;
        }
    }

    async loadDashboard() {
        try {
            const stats = await this.api.getStatistics();
            const status = await this.api.getServerStatus();
            
            this.currentStats = stats;
            this.updateStatsCards(stats, status);
            this.updateCharts(stats);
            this.updateServerStatus(status);
        } catch (error) {
            console.error('Failed to load dashboard:', error);
            this.showToast('Failed to load dashboard', 'error');
        }
    }

    updateStatsCards(stats, status) {
        // Server status
        const statusEl = document.getElementById('statStatus');
        const uptimeEl = document.getElementById('statUptime');
        if (statusEl) {
            statusEl.textContent = status.state || 'Unknown';
            statusEl.style.color = status.state === 'running' ? 'var(--success)' : 
                                   status.state === 'error' ? 'var(--danger)' : 'var(--text-secondary)';
        }
        if (uptimeEl) {
            uptimeEl.textContent = `Uptime: ${this.formatUptime(status.uptime_seconds || 0)}`;
        }

        // Connections
        const connEl = document.getElementById('statConnections');
        const connSecEl = document.getElementById('statConnPerSec');
        if (connEl) connEl.textContent = stats.active_connections?.toLocaleString() || '0';
        if (connSecEl) {
            const cps = stats.connections_per_second || 0;
            connSecEl.textContent = `${cps > 0 ? '+' : ''}${cps}/сек`;
            connSecEl.className = 'stat-change ' + (cps > 0 ? 'positive' : '');
        }

        // Traffic
        const trafficEl = document.getElementById('statTraffic');
        const trafficSecEl = document.getElementById('statTrafficSec');
        if (trafficEl) {
            const total = (stats.bytes_sent || 0) + (stats.bytes_received || 0);
            trafficEl.textContent = this.formatBytes(total);
        }
        if (trafficSecEl) {
            const bps = (stats.bytes_per_second_in || 0) + (stats.bytes_per_second_out || 0);
            trafficSecEl.textContent = `${this.formatBytes(bps)}/сек`;
        }

        // CPU & Memory
        const cpuEl = document.getElementById('statCpu');
        const memEl = document.getElementById('statMemory');
        if (cpuEl) cpuEl.textContent = `${(stats.cpu_usage_percent || 0).toFixed(1)}%`;
        if (memEl) memEl.textContent = `Память: ${this.formatBytes(stats.memory_usage_bytes || 0)}`;
    }

    updateCharts(stats) {
        // Add to history
        this.statsHistory.push({
            timestamp: Date.now(),
            connections: stats.active_connections || 0,
            connPerSec: stats.connections_per_second || 0,
            trafficIn: stats.bytes_per_second_in || 0,
            trafficOut: stats.bytes_per_second_out || 0,
        });

        // Limit history
        if (this.statsHistory.length > this.maxHistoryPoints) {
            this.statsHistory.shift();
        }

        // Update charts (simple implementation)
        this.renderConnectionsChart();
        this.renderTrafficChart();
    }

    renderConnectionsChart() {
        const chartEl = document.getElementById('connectionsChart');
        if (!chartEl || this.statsHistory.length === 0) return;

        const maxConn = Math.max(...this.statsHistory.map(s => s.connections), 1);
        const bars = this.statsHistory.map(s => {
            const height = (s.connections / maxConn) * 100;
            return `<div class="chart-bar" style="height: ${height}%" title="${s.connections}"></div>`;
        }).join('');

        chartEl.innerHTML = `
            <div class="chart-bars">
                ${bars}
            </div>
        `;
    }

    renderTrafficChart() {
        const chartEl = document.getElementById('trafficChart');
        if (!chartEl || this.statsHistory.length === 0) return;

        const maxTraffic = Math.max(
            ...this.statsHistory.map(s => Math.max(s.trafficIn, s.trafficOut)),
            1
        );
        
        const bars = this.statsHistory.map(s => {
            const inHeight = (s.trafficIn / maxTraffic) * 100;
            const outHeight = (s.trafficOut / maxTraffic) * 100;
            return `
                <div class="chart-bar-group">
                    <div class="chart-bar chart-bar-in" style="height: ${inHeight}%" title="In: ${this.formatBytes(s.trafficIn)}/s"></div>
                    <div class="chart-bar chart-bar-out" style="height: ${outHeight}%" title="Out: ${this.formatBytes(s.trafficOut)}/s"></div>
                </div>
            `;
        }).join('');

        chartEl.innerHTML = `
            <div class="chart-bars">
                ${bars}
            </div>
            <div class="chart-legend">
                <span class="legend-item"><span class="legend-color in"></span> Входящий</span>
                <span class="legend-item"><span class="legend-color out"></span> Исходящий</span>
            </div>
        `;
    }

    updateServerStatus(status) {
        const indicator = document.querySelector('.status-indicator');
        const text = document.querySelector('.status-text');
        
        if (indicator) {
            indicator.className = 'status-indicator ' + (status.state || 'unknown');
        }
        if (text) {
            text.textContent = status.state ? status.state.charAt(0).toUpperCase() + status.state.slice(1) : 'Unknown';
        }
    }

    async loadConfig() {
        try {
            const config = await this.api.getConfig();
            
            document.getElementById('configPort').value = config.port || 8888;
            document.getElementById('configMaxConn').value = config.max_connections || 10000;
            document.getElementById('configIpv6').checked = config.ipv6_enabled || false;
            document.getElementById('configLogLevel').value = config.log_level || 'INFO';
            document.getElementById('configLogFile').value = config.log_file || '/var/log/mtproxy.log';
            document.getElementById('configWorkers').value = config.workers || 1;
            document.getElementById('configAesNi').checked = config.use_aes_ni !== false;
            
            this.showToast('Configuration loaded', 'success');
        } catch (error) {
            console.error('Failed to load config:', error);
            this.showToast('Failed to load configuration', 'error');
        }
    }

    async saveConfig() {
        try {
            const config = {
                port: parseInt(document.getElementById('configPort').value),
                max_connections: parseInt(document.getElementById('configMaxConn').value),
                ipv6_enabled: document.getElementById('configIpv6').checked,
                log_level: document.getElementById('configLogLevel').value,
                log_file: document.getElementById('configLogFile').value,
                workers: parseInt(document.getElementById('configWorkers').value),
                use_aes_ni: document.getElementById('configAesNi').checked,
            };

            await this.api.updateConfig(config);
            this.showToast('Configuration saved', 'success');
        } catch (error) {
            console.error('Failed to save config:', error);
            this.showToast('Failed to save configuration', 'error');
        }
    }

    async loadSecrets() {
        const container = document.getElementById('secretsList');
        if (!container) return;

        try {
            const data = await this.api.getSecrets();
            const secrets = data.secrets || [];

            if (secrets.length === 0) {
                container.innerHTML = '<div class="loading">No secrets configured</div>';
                return;
            }

            container.innerHTML = secrets.map(secret => `
                <div class="secret-item">
                    <div class="secret-info">
                        <div class="secret-value">${this.maskSecret(secret.secret_hash || secret.secret)}</div>
                        ${secret.description ? `<div class="secret-description">${secret.description}</div>` : ''}
                        <div class="secret-meta">
                            <span>Created: ${this.formatDate(secret.created_at_unix)}</span>
                            <span>Used: ${secret.usage_count || 0} times</span>
                        </div>
                    </div>
                    <div class="secret-actions">
                        <button class="btn btn-sm btn-secondary" onclick="navigator.clipboard.writeText('${secret.secret || ''}')">
                            Copy
                        </button>
                        <button class="btn btn-sm btn-danger" onclick="app.deleteSecret('${secret.secret || ''}')">
                            Delete
                        </button>
                    </div>
                </div>
            `).join('');
        } catch (error) {
            console.error('Failed to load secrets:', error);
            container.innerHTML = '<div class="loading">Failed to load secrets</div>';
        }
    }

    async handleAddSecret(e) {
        e.preventDefault();
        
        const secret = document.getElementById('secretValue').value;
        const description = document.getElementById('secretDescription').value;

        try {
            await this.api.addSecret(secret, description);
            this.closeModal('addSecretModal');
            document.getElementById('addSecretForm').reset();
            this.showToast('Secret added successfully', 'success');
            this.loadSecrets();
        } catch (error) {
            console.error('Failed to add secret:', error);
            this.showToast('Failed to add secret', 'error');
        }
    }

    async deleteSecret(secret) {
        if (!confirm('Are you sure you want to delete this secret?')) return;

        try {
            await this.api.removeSecret(secret);
            this.showToast('Secret deleted', 'success');
            this.loadSecrets();
        } catch (error) {
            console.error('Failed to delete secret:', error);
            this.showToast('Failed to delete secret', 'error');
        }
    }

    async loadLogs() {
        const container = document.getElementById('logsContainer');
        const levelFilter = document.getElementById('logLevelFilter')?.value || '';
        
        if (!container) return;

        try {
            const data = await this.api.getLogs(100, levelFilter);
            const logs = data.entries || [];

            if (logs.length === 0) {
                container.innerHTML = '<div class="loading">No logs found</div>';
                return;
            }

            container.innerHTML = logs.map(log => `
                <div class="log-entry">
                    <span class="log-timestamp">${this.formatDate(log.timestamp_unix)}</span>
                    <span class="log-level ${log.level}">${log.level}</span>
                    <span class="log-message">${log.message}</span>
                </div>
            `).join('');
        } catch (error) {
            console.error('Failed to load logs:', error);
            container.innerHTML = '<div class="loading">Failed to load logs</div>';
        }
    }

    // Utilities
    formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const mins = Math.floor((seconds % 3600) / 60);
        
        if (days > 0) return `${days}d ${hours}h ${mins}m`;
        if (hours > 0) return `${hours}h ${mins}m`;
        return `${mins}m`;
    }

    formatDate(timestamp) {
        if (!timestamp) return '-';
        return new Date(timestamp * 1000).toLocaleString();
    }

    maskSecret(secret) {
        if (!secret) return '';
        if (secret.length <= 16) return '***';
        return secret.substring(0, 8) + '...' + secret.substring(secret.length - 8);
    }

    showToast(message, type = 'info') {
        const container = document.getElementById('toastContainer');
        if (!container) return;

        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        toast.textContent = message;

        container.appendChild(toast);

        setTimeout(() => {
            toast.style.animation = 'slideIn 0.3s ease reverse';
            setTimeout(() => toast.remove(), 300);
        }, 3000);
    }

    openModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) modal.classList.add('active');
    }

    closeModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) modal.classList.remove('active');
    }
}

// Initialize app
const app = new MTProxyApp();
