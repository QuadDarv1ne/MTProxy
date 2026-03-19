import 'package:flutter_test/flutter_test.dart';
import 'package:mtproxy_mobile/utils/format_utils.dart';

void main() {
  group('FormatUtils', () {
    group('formatBytes', () {
      test('formats bytes correctly', () {
        expect(FormatUtils.formatBytes(0), '0 B');
        expect(FormatUtils.formatBytes(512), '512 B');
        expect(FormatUtils.formatBytes(1024), '1.0 KB');
        expect(FormatUtils.formatBytes(1536), '1.5 KB');
        expect(FormatUtils.formatBytes(1048576), '1.0 MB');
        expect(FormatUtils.formatBytes(1073741824), '1.0 GB');
      });

      test('returns 0 B for negative values', () {
        expect(FormatUtils.formatBytes(-100), '0 B');
      });
    });

    group('formatDuration', () {
      test('formats duration correctly', () {
        expect(
          FormatUtils.formatDuration(const Duration(seconds: 5)),
          '5с',
        );
        expect(
          FormatUtils.formatDuration(const Duration(minutes: 2, seconds: 30)),
          '2м 30с',
        );
        expect(
          FormatUtils.formatDuration(const Duration(hours: 1, minutes: 30)),
          '1ч 30м 0с',
        );
        expect(
          FormatUtils.formatDuration(
            const Duration(days: 1, hours: 2, minutes: 15),
          ),
          '1д 2ч 15м',
        );
      });
    });

    group('formatPercent', () {
      test('formats percent correctly', () {
        expect(FormatUtils.formatPercent(75.5), '75.5%');
        expect(FormatUtils.formatPercent(75.55, decimals: 2), '75.55%');
        expect(FormatUtils.formatPercent(100.0), '100.0%');
        expect(FormatUtils.formatPercent(0.0), '0.0%');
      });
    });

    group('formatNumber', () {
      test('formats numbers with thousand separators', () {
        expect(FormatUtils.formatNumber(1000), '1 000');
        expect(FormatUtils.formatNumber(1000000), '1 000 000');
        expect(FormatUtils.formatNumber(1234567), '1 234 567');
        expect(FormatUtils.formatNumber(999), '999');
      });
    });

    group('formatHexSecret', () {
      test('formats hex secret with truncation', () {
        const secret = '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef';
        expect(
          FormatUtils.formatHexSecret(secret),
          '01234567...89abcdef',
        );
      });

      test('shows full secret if short enough', () {
        const secret = '01234567';
        expect(FormatUtils.formatHexSecret(secret), '01234567');
      });
    });

    group('maskSecret', () {
      test('masks secret correctly', () {
        const secret = '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef';
        expect(
          FormatUtils.maskSecret(secret),
          '0123' + ('*' * 56) + 'cdef',
        );
      });
    });

    group('getStatusColor', () {
      test('returns correct colors for statuses', () {
        expect(FormatUtils.getStatusColor('running'), 0xFF4CAF50);
        expect(FormatUtils.getStatusColor('stopped'), 0xFF9E9E9E);
        expect(FormatUtils.getStatusColor('error'), 0xFFF44336);
        expect(FormatUtils.getStatusColor('starting'), 0xFFFFC107);
        expect(FormatUtils.getStatusColor('unknown'), 0xFF9E9E9E);
      });
    });
  });
}
