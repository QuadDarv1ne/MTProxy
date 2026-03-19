import 'package:flutter_test/flutter_test.dart';
import 'package:mtproxy_mobile/utils/validation_utils.dart';

void main() {
  group('ValidationUtils', () {
    group('validatePort', () {
      test('validates correct ports', () {
        expect(ValidationUtils.validatePort('443'), isNull);
        expect(ValidationUtils.validatePort('8080'), isNull);
        expect(ValidationUtils.validatePort('65535'), isNull);
      });

      test('rejects empty values', () {
        expect(ValidationUtils.validatePort(''), isNotNull);
        expect(ValidationUtils.validatePort(null), isNotNull);
      });

      test('rejects non-numeric values', () {
        expect(ValidationUtils.validatePort('abc'), isNotNull);
        expect(ValidationUtils.validatePort('12.34'), isNotNull);
      });

      test('rejects out of range ports', () {
        expect(ValidationUtils.validatePort('0'), isNotNull);
        expect(ValidationUtils.validatePort('-1'), isNotNull);
        expect(ValidationUtils.validatePort('65536'), isNotNull);
      });

      test('warns about privileged ports', () {
        expect(ValidationUtils.validatePort('80'), isNotNull);
        expect(ValidationUtils.validatePort('22'), isNotNull);
      });
    });

    group('validateHexSecret', () {
      test('validates correct secrets', () {
        const validSecret =
            '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef';
        expect(ValidationUtils.validateHexSecret(validSecret), isNull);
        expect(
          ValidationUtils.validateHexSecret(validSecret.toUpperCase()),
          isNull,
        );
      });

      test('rejects empty values when not allowed', () {
        expect(ValidationUtils.validateHexSecret(''), isNotNull);
        expect(ValidationUtils.validateHexSecret(null), isNotNull);
        expect(ValidationUtils.validateHexSecret('', allowEmpty: true), isNull);
      });

      test('rejects wrong length', () {
        expect(ValidationUtils.validateHexSecret('abc'), isNotNull);
        expect(
          ValidationUtils.validateHexSecret('0123456789abcdef'),
          isNotNull,
        );
        expect(
          ValidationUtils.validateHexSecret(
            '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef00',
          ),
          isNotNull,
        );
      });

      test('rejects non-hex characters', () {
        expect(ValidationUtils.validateHexSecret('g' * 64), isNotNull);
        expect(ValidationUtils.validateHexSecret('Z' * 64), isNotNull);
        expect(ValidationUtils.validateHexSecret('!' * 64), isNotNull);
      });
    });

    group('validateIpv4', () {
      test('validates correct IPv4 addresses', () {
        expect(ValidationUtils.validateIpv4('192.168.1.1'), isNull);
        expect(ValidationUtils.validateIpv4('0.0.0.0'), isNull);
        expect(ValidationUtils.validateIpv4('255.255.255.255'), isNull);
        expect(ValidationUtils.validateIpv4('10.0.0.1'), isNull);
      });

      test('rejects invalid IPv4 addresses', () {
        expect(ValidationUtils.validateIpv4('256.1.1.1'), isNotNull);
        expect(ValidationUtils.validateIpv4('1.256.1.1'), isNotNull);
        expect(ValidationUtils.validateIpv4('1.1.1'), isNotNull);
        expect(ValidationUtils.validateIpv4('1.1.1.1.1'), isNotNull);
        expect(ValidationUtils.validateIpv4('abc.def.ghi.jkl'), isNotNull);
      });
    });

    group('validateEmail', () {
      test('validates correct emails', () {
        expect(ValidationUtils.validateEmail('test@example.com'), isNull);
        expect(ValidationUtils.validateEmail('user.name@domain.co.uk'), isNull);
        expect(ValidationUtils.validateEmail('test123@test.org'), isNull);
      });

      test('rejects invalid emails', () {
        expect(ValidationUtils.validateEmail('invalid'), isNotNull);
        expect(ValidationUtils.validateEmail('@example.com'), isNotNull);
        expect(ValidationUtils.validateEmail('test@'), isNotNull);
        expect(ValidationUtils.validateEmail('test@example'), isNotNull);
      });
    });

    group('validateNumber', () {
      test('validates numbers in range', () {
        expect(
          ValidationUtils.validateNumber('50', min: 0, max: 100),
          isNull,
        );
        expect(
          ValidationUtils.validateNumber('0', min: 0, max: 100),
          isNull,
        );
        expect(
          ValidationUtils.validateNumber('100', min: 0, max: 100),
          isNull,
        );
      });

      test('rejects numbers out of range', () {
        expect(
          ValidationUtils.validateNumber('-1', min: 0, max: 100),
          isNotNull,
        );
        expect(
          ValidationUtils.validateNumber('101', min: 0, max: 100),
          isNotNull,
        );
      });

      test('rejects non-numeric values', () {
        expect(
          ValidationUtils.validateNumber('abc', min: 0, max: 100),
          isNotNull,
        );
      });
    });

    group('validateStringLength', () {
      test('validates strings with correct length', () {
        expect(
          ValidationUtils.validateStringLength(
            'hello',
            minLength: 1,
            maxLength: 10,
          ),
          isNull,
        );
      });

      test('rejects strings that are too short', () {
        expect(
          ValidationUtils.validateStringLength(
            '',
            minLength: 1,
            maxLength: 10,
          ),
          isNotNull,
        );
      });

      test('rejects strings that are too long', () {
        expect(
          ValidationUtils.validateStringLength(
            'this is a very long string',
            minLength: 1,
            maxLength: 10,
          ),
          isNotNull,
        );
      });
    });

    group('validateListNotEmpty', () {
      test('validates non-empty lists', () {
        expect(
          ValidationUtils.validateListNotEmpty([1, 2, 3], 'Items'),
          isNull,
        );
      });

      test('rejects empty lists', () {
        expect(
          ValidationUtils.validateListNotEmpty([], 'Items'),
          isNotNull,
        );
        expect(
          ValidationUtils.validateListNotEmpty(null, 'Items'),
          isNotNull,
        );
      });
    });
  });

  group('ValidationExtensions', () {
    test('string extensions work correctly', () {
      expect('443'.isValidPort, isTrue);
      expect('99999'.isValidPort, isFalse);

      const validSecret =
          '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef';
      expect(validSecret.isValidHexSecret, isTrue);
      expect('invalid'.isValidHexSecret, isFalse);

      expect('192.168.1.1'.isValidIpv4, isTrue);
      expect('256.1.1.1'.isValidIpv4, isFalse);

      expect('test@example.com'.isValidEmail, isTrue);
      expect('invalid'.isValidEmail, isFalse);
    });
  });
}
