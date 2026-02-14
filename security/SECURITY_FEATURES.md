# Security Features for MTProxy

## Overview

This document describes the security enhancements implemented in MTProxy to improve protection against various threats and attacks.

## Features

### 1. Certificate Pinning

Certificate pinning is a security feature that protects against man-in-the-middle (MITM) attacks by ensuring that MTProxy only accepts connections from servers with specific, pre-approved certificates.

#### Implementation
- Uses SHA256 hashes of certificates for comparison
- Supports up to 100 pinned certificates
- Validates certificates at connection time
- Allows dynamic addition/removal of pinned certificates

#### Usage
The certificate pinning system is initialized during startup and automatically validates all upstream connections against pinned certificates.

### 2. DDoS Protection

Distributed Denial of Service (DDoS) protection mechanisms to prevent service disruption from coordinated attacks.

#### Features
- Rate limiting per IP address
- Connection throttling
- IP blocking capabilities
- Attack pattern detection
- Configurable thresholds

#### Configuration
- Maximum connections per IP: 100 (default)
- Rate limit window: 60 seconds
- Block duration: 300 seconds
- IP blocking: Enabled

### 3. HSM Integration (Planned)

Hardware Security Module (HSM) integration for secure key storage and cryptographic operations.

#### Planned Features
- PKCS#11 interface
- Secure key generation
- Hardware-accelerated encryption
- Tamper-resistant key storage

## Architecture

The security features are implemented as separate modules that integrate with the main MTProxy codebase:

```
security/
├── security-manager.c/h    # Main security orchestration
├── ddos-protection.c/h     # DDoS protection mechanisms
├── cert-pinning.c/h        # Certificate pinning system
└── SECURITY_FEATURES.md    # This documentation
```

## Integration

Security features are integrated into the build system with both CMake and Makefile support. The modules are designed to work seamlessly with existing MTProxy code without breaking changes.

## Performance Impact

Security features are designed to have minimal impact on performance:
- Certificate validation occurs asynchronously
- DDoS protection uses efficient data structures
- Memory usage is optimized for large-scale deployments

## Security Levels

The system supports configurable security levels:
- Level 0: Minimal security (for testing)
- Level 1: Standard security (recommended for most deployments)
- Level 2: High security (for sensitive deployments)
- Level 3: Maximum security (with potential performance impact)