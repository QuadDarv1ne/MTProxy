/**
 * MTProxy REST API Client
 */

const API_BASE_URL = window.location.origin;
const API_KEY = localStorage.getItem('mtproxy_api_key') || '';

class MTProxyAPI {
    constructor() {
        this.baseUrl = API_BASE_URL;
        this.apiKey = API_KEY;
    }

    async request(endpoint, options = {}) {
        const url = `${this.baseUrl}${endpoint}`;
        const headers = {
            'Content-Type': 'application/json',
            ...options.headers,
        };

        if (this.apiKey) {
            headers['X-API-Key'] = this.apiKey;
        }

        try {
            const response = await fetch(url, {
                ...options,
                headers,
            });

            if (!response.ok) {
                if (response.status === 401) {
                    throw new Error('Unauthorized. Please check your API key.');
                }
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }

            return await response.json();
        } catch (error) {
            console.error('API request failed:', error);
            throw error;
        }
    }

    // Server Management
    async getServerStatus() {
        return this.request('/api/v1/server/status');
    }

    async startServer(force = false) {
        return this.request('/api/v1/server/start', {
            method: 'POST',
            body: JSON.stringify({ force }),
        });
    }

    async stopServer(graceful = true, timeoutSeconds = 30) {
        return this.request('/api/v1/server/stop', {
            method: 'POST',
            body: JSON.stringify({ graceful, timeout_seconds: timeoutSeconds }),
        });
    }

    async restartServer(graceful = true, timeoutSeconds = 30) {
        return this.request('/api/v1/server/restart', {
            method: 'POST',
            body: JSON.stringify({ graceful, timeout_seconds: timeoutSeconds }),
        });
    }

    // Statistics
    async getStatistics() {
        return this.request('/api/v1/statistics');
    }

    // Configuration
    async getConfig() {
        return this.request('/api/v1/config');
    }

    async updateConfig(config) {
        return this.request('/api/v1/config', {
            method: 'PUT',
            body: JSON.stringify(config),
        });
    }

    async validateConfig(config) {
        return this.request('/api/v1/config/validate', {
            method: 'POST',
            body: JSON.stringify(config),
        });
    }

    // Secrets
    async getSecrets() {
        return this.request('/api/v1/secrets');
    }

    async addSecret(secret, description = '') {
        return this.request('/api/v1/secrets', {
            method: 'POST',
            body: JSON.stringify({ secret, description }),
        });
    }

    async removeSecret(secret) {
        return this.request(`/api/v1/secrets/${encodeURIComponent(secret)}`, {
            method: 'DELETE',
        });
    }

    // Connections
    async getConnections(limit = 100, offset = 0) {
        return this.request(`/api/v1/connections?limit=${limit}&offset=${offset}`);
    }

    // Logs
    async getLogs(limit = 100, level = '', startTime = 0, endTime = 0) {
        const params = new URLSearchParams({ limit: limit.toString() });
        if (level) params.append('level', level);
        if (startTime) params.append('start_time', startTime.toString());
        if (endTime) params.append('end_time', endTime.toString());
        return this.request(`/api/v1/logs?${params.toString()}`);
    }

    // Rate Limiting
    async getRateLimits() {
        return this.request('/api/v1/ratelimit');
    }

    async updateRateLimit(config) {
        return this.request('/api/v1/ratelimit', {
            method: 'PUT',
            body: JSON.stringify(config),
        });
    }
}

// Export singleton instance
window.mtproxyApi = new MTProxyAPI();
