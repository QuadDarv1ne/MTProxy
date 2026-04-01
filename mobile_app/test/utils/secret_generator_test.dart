import 'package:flutter_test/flutter_test.dart';
import 'package:mtproxy_mobile/utils/secret_generator.dart';

void main() {
  group('SecretGenerator', () {
    group('generateHexSecret', () {
      test('generates 64 character hex string', () {
        final secret = SecretGenerator.generateHexSecret();
        expect(secret.length, 64);
        expect(RegExp(r'^[0-9a-f]+$').hasMatch(secret), isTrue);
      });

      test('generates unique secrets', () {
        final secrets = Set<String>();
        for (var i = 0; i < 100; i++) {
          secrets.add(SecretGenerator.generateHexSecret());
        }
        expect(secrets.length, 100);
      });
    });

    group('generateSecrets', () {
      test('generates correct number of secrets', () {
        final secrets = SecretGenerator.generateSecrets(5);
        expect(secrets.length, 5);
        for (final secret in secrets) {
          expect(secret.length, 64);
        }
      });
    });

    group('generateSecretWithLength', () {
      test('generates secret with specified length', () {
        expect(SecretGenerator.generateSecretWithLength(16).length, 32);
        expect(SecretGenerator.generateSecretWithLength(32).length, 64);
        expect(SecretGenerator.generateSecretWithLength(64).length, 128);
      });
    });

    group('randomInt', () {
      test('generates number in range', () {
        for (var i = 0; i < 100; i++) {
          final num = SecretGenerator.randomInt(1, 10);
          expect(num >= 1 && num <= 10, isTrue);
        }
      });

      test('throws on invalid range', () {
        expect(
          () => SecretGenerator.randomInt(10, 5),
          throwsArgumentError,
        );
      });
    });

    group('randomPort', () {
      test('generates valid port numbers', () {
        for (var i = 0; i < 10; i++) {
          final port = SecretGenerator.randomPort();
          expect(port > 0 && port <= 65535, isTrue);
        }
      });
    });

    group('generateId', () {
      test('generates ID with default length', () {
        final id = SecretGenerator.generateId();
        expect(id.length, 16);
      });

      test('generates ID with specified length', () {
        expect(SecretGenerator.generateId(length: 8).length, 8);
        expect(SecretGenerator.generateId(length: 32).length, 32);
      });

      test('generates unique IDs', () {
        final ids = Set<String>();
        for (var i = 0; i < 100; i++) {
          ids.add(SecretGenerator.generateId());
        }
        expect(ids.length, 100);
      });
    });

    group('generatePassword', () {
      test('generates password with default settings', () {
        final password = SecretGenerator.generatePassword();
        expect(password.length, 32);
      });

      test('generates password with specified length', () {
        expect(SecretGenerator.generatePassword(length: 16).length, 16);
      });

      test('generates password with only lowercase', () {
        final password = SecretGenerator.generatePassword(
          length: 100,
          includeUppercase: false,
          includeNumbers: false,
          includeSymbols: false,
        );
        expect(RegExp(r'^[a-z]+$').hasMatch(password), isTrue);
      });

      test('generates password with only numbers', () {
        final password = SecretGenerator.generatePassword(
          length: 100,
          includeUppercase: false,
          includeLowercase: false,
          includeSymbols: false,
        );
        expect(RegExp(r'^[0-9]+$').hasMatch(password), isTrue);
      });

      test('throws when no character types selected', () {
        expect(
          () => SecretGenerator.generatePassword(
            includeUppercase: false,
            includeLowercase: false,
            includeNumbers: false,
            includeSymbols: false,
          ),
          throwsArgumentError,
        );
      });
    });

    group('shuffleList', () {
      test('shuffles list', () {
        final original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
        final shuffled = SecretGenerator.shuffleList(original);
        expect(shuffled.length, original.length);
        expect(shuffled.toSet(), original.toSet());
      });

      test('handles empty list', () {
        expect(SecretGenerator.shuffleList([]), isEmpty);
      });
    });

    group('randomElement', () {
      test('returns element from list', () {
        final list = [1, 2, 3, 4, 5];
        final element = SecretGenerator.randomElement(list);
        expect(list.contains(element), isTrue);
      });

      test('throws on empty list', () {
        expect(
          () => SecretGenerator.randomElement([]),
          throwsStateError,
        );
      });
    });

    group('randomBool', () {
      test('returns boolean values', () {
        for (var i = 0; i < 100; i++) {
          final result = SecretGenerator.randomBool();
          expect(result, isA<bool>());
        }
      });

      test('respects probability', () {
        var trueCount = 0;
        for (var i = 0; i < 1000; i++) {
          if (SecretGenerator.randomBool(probability: 0.8)) {
            trueCount++;
          }
        }
        // Should be approximately 800 (allowing for randomness)
        expect(trueCount > 700 && trueCount < 900, isTrue);
      });

      test('throws on invalid probability', () {
        expect(
          () => SecretGenerator.randomBool(probability: -0.1),
          throwsArgumentError,
        );
        expect(
          () => SecretGenerator.randomBool(probability: 1.1),
          throwsArgumentError,
        );
      });
    });
  });
}
