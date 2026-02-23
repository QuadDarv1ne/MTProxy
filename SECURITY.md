# Security Policy

## Supported Versions

We release patches for security vulnerabilities for the following versions:

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

We take the security of MTProxy seriously. If you believe you have found a security vulnerability, please report it to us as described below.

### Where to Report

**Please DO NOT report security vulnerabilities through public GitHub issues.**

Instead, please report them via:

1. **Telegram (Preferred):**
   - [@quadd4rv1n7](https://t.me/quadd4rv1n7)
   - [@dupley_maxim_1999](https://t.me/dupley_maxim_1999)

2. **Email:** (if you prefer email communication)
   - Contact through the Telegram channels above to get a secure email address

### What to Include

Please include the following information in your report:

- Type of vulnerability
- Full paths of source file(s) related to the vulnerability
- Location of the affected source code (tag/branch/commit or direct URL)
- Step-by-step instructions to reproduce the issue
- Proof-of-concept or exploit code (if possible)
- Impact of the vulnerability, including how an attacker might exploit it

### Response Timeline

- **Initial Response:** Within 48 hours
- **Status Update:** Within 7 days
- **Fix Timeline:** Depends on severity
  - Critical: 1-7 days
  - High: 7-30 days
  - Medium: 30-90 days
  - Low: Best effort

## Security Best Practices

### Deployment

1. **Run as Non-Root User**
   ```bash
   # Always use -u flag with a non-privileged user
   ./mtproto-proxy -u nobody ...
   ```

2. **Restrict Statistics Port**
   ```bash
   # Bind statistics only to localhost
   ./mtproto-proxy -p 8888 ...
   # Use firewall to block external access
   iptables -A INPUT -p tcp --dport 8888 ! -s 127.0.0.1 -j DROP
   ```

3. **File Permissions**
   ```bash
   # Protect secret files
   chmod 600 proxy-secret
   chmod 600 proxy-multi.conf
   chown mtproxy:mtproxy proxy-secret proxy-multi.conf
   ```

4. **Regular Updates**
   ```bash
   # Update Telegram configuration daily
   curl -s https://core.telegram.org/getProxySecret -o proxy-secret
   curl -s https://core.telegram.org/getProxyConfig -o proxy-multi.conf
   ```

### Docker Security

1. **Use Non-Root User**
   - The provided Dockerfile already runs as non-root user `mtproxy`

2. **Limit Resources**
   ```yaml
   # In docker-compose.yml
   deploy:
     resources:
       limits:
         cpus: '2'
         memory: 1G
   ```

3. **Read-Only Filesystem**
   ```yaml
   # Add to docker-compose.yml
   read_only: true
   tmpfs:
     - /tmp
   ```

4. **Drop Capabilities**
   ```yaml
   # Add to docker-compose.yml
   cap_drop:
     - ALL
   cap_add:
     - NET_BIND_SERVICE
   ```

### Network Security

1. **Firewall Configuration**
   ```bash
   # Allow only necessary ports
   ufw allow 443/tcp    # MTProxy
   ufw deny 8888/tcp    # Statistics (use SSH tunnel instead)
   ufw enable
   ```

2. **DDoS Protection**
   - Enable built-in DDoS protection features
   - Use rate limiting
   - Consider using a CDN or DDoS protection service

3. **TLS/SSL**
   - Use random padding mode (dd prefix) to avoid detection
   - Keep OpenSSL updated

### Monitoring

1. **Log Monitoring**
   ```bash
   # Monitor for suspicious activity
   tail -f /var/log/mtproxy.log | grep -i "error\|fail\|attack"
   ```

2. **Statistics Monitoring**
   ```bash
   # Regular health checks
   curl -s http://localhost:8888/stats
   ```

3. **Resource Monitoring**
   ```bash
   # Monitor CPU, memory, network usage
   top -p $(pgrep mtproto-proxy)
   ```

## Known Security Features

### Built-in Protection

- **Certificate Pinning:** Protection against MITM attacks
- **DDoS Protection:** Rate limiting and connection filtering
- **Random Padding:** Obfuscation to avoid detection
- **Memory Protection:** Stack protection and buffer overflow prevention

### Cryptographic Features

- **AES-256 Encryption:** Strong encryption for data in transit
- **Perfect Forward Secrecy:** MTProto v3 support
- **Hardware Acceleration:** AES-NI support for better performance

## Security Advisories

Security advisories will be published in:
- GitHub Security Advisories
- README.md updates
- Release notes

## Disclosure Policy

- We follow responsible disclosure practices
- We will acknowledge your contribution (unless you prefer to remain anonymous)
- We aim to coordinate disclosure timing with you
- We will credit researchers in security advisories

## Security Hall of Fame

We maintain a list of security researchers who have responsibly disclosed vulnerabilities:

<!-- Add researchers here -->
- *No vulnerabilities reported yet*

## Additional Resources

- [MTProto Protocol Documentation](https://core.telegram.org/mtproto)
- [Telegram Security Guidelines](https://core.telegram.org/api/security-guidelines)
- [OWASP Secure Coding Practices](https://owasp.org/www-project-secure-coding-practices-quick-reference-guide/)

---

Thank you for helping keep MTProxy and its users safe! 🔒
