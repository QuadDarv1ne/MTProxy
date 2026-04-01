import 'package:flutter_test/flutter_test.dart';
import 'package:mtproxy_mobile/services/mtproxy_error_handler.dart';

void main() {
  group('MtProxyErrorInterpreter', () {
    group('interpret', () {
      test('returns success message for code 0', () {
        expect(
          MtProxyErrorInterpreter.interpret(0),
          'Операция выполнена успешно',
        );
      });

      test('interprets known error codes', () {
        expect(
          MtProxyErrorInterpreter.interpret(-1),
          'MTProxy не инициализирован',
        );
        expect(
          MtProxyErrorInterpreter.interpret(-2),
          'MTProxy уже запущен',
        );
        expect(
          MtProxyErrorInterpreter.interpret(-3),
          'Необходимо добавить хотя бы один секретный ключ',
        );
        expect(
          MtProxyErrorInterpreter.interpret(-4),
          'Ошибка выделения памяти',
        );
        expect(
          MtProxyErrorInterpreter.interpret(-9),
          'Порт уже используется',
        );
      });

      test('interprets unknown negative codes', () {
        expect(
          MtProxyErrorInterpreter.interpret(-99),
          'Неизвестная ошибка: -99',
        );
      });

      test('interprets positive codes as system errors', () {
        expect(
          MtProxyErrorInterpreter.interpret(5),
          'Системная ошибка: 5',
        );
      });
    });

    group('interpretType', () {
      test('returns unknown for code 0', () {
        expect(
          MtProxyErrorInterpreter.interpretType(0),
          MtProxyErrorType.unknown,
        );
      });

      test('classifies initialization errors', () {
        expect(
          MtProxyErrorInterpreter.interpretType(-1),
          MtProxyErrorType.initialization,
        );
        expect(
          MtProxyErrorInterpreter.interpretType(-4),
          MtProxyErrorType.initialization,
        );
      });

      test('classifies configuration errors', () {
        expect(
          MtProxyErrorInterpreter.interpretType(-3),
          MtProxyErrorType.configuration,
        );
        expect(
          MtProxyErrorInterpreter.interpretType(-5),
          MtProxyErrorType.configuration,
        );
      });

      test('classifies startup errors', () {
        expect(
          MtProxyErrorInterpreter.interpretType(-2),
          MtProxyErrorType.startup,
        );
        expect(
          MtProxyErrorInterpreter.interpretType(-8),
          MtProxyErrorType.startup,
        );
      });

      test('classifies runtime errors', () {
        expect(
          MtProxyErrorInterpreter.interpretType(-6),
          MtProxyErrorType.runtime,
        );
        expect(
          MtProxyErrorInterpreter.interpretType(-7),
          MtProxyErrorType.runtime,
        );
      });

      test('classifies platform errors', () {
        expect(
          MtProxyErrorInterpreter.interpretType(-10),
          MtProxyErrorType.platform,
        );
      });
    });

    group('isCritical', () {
      test('returns false for success', () {
        expect(MtProxyErrorInterpreter.isCritical(0), isFalse);
      });

      test('identifies critical errors', () {
        expect(MtProxyErrorInterpreter.isCritical(-1), isTrue);
        expect(MtProxyErrorInterpreter.isCritical(-4), isTrue);
        expect(MtProxyErrorInterpreter.isCritical(-6), isTrue);
        expect(MtProxyErrorInterpreter.isCritical(-7), isTrue);
      });

      test('returns false for non-critical errors', () {
        expect(MtProxyErrorInterpreter.isCritical(-2), isFalse);
        expect(MtProxyErrorInterpreter.isCritical(-3), isFalse);
        expect(MtProxyErrorInterpreter.isCritical(-5), isFalse);
      });
    });

    group('isRecoverable', () {
      test('returns false for success', () {
        expect(MtProxyErrorInterpreter.isRecoverable(0), isFalse);
      });

      test('identifies recoverable errors', () {
        expect(MtProxyErrorInterpreter.isRecoverable(-2), isTrue);
        expect(MtProxyErrorInterpreter.isRecoverable(-3), isTrue);
        expect(MtProxyErrorInterpreter.isRecoverable(-5), isTrue);
        expect(MtProxyErrorInterpreter.isRecoverable(-9), isTrue);
      });

      test('returns false for non-recoverable errors', () {
        expect(MtProxyErrorInterpreter.isRecoverable(-1), isFalse);
        expect(MtProxyErrorInterpreter.isRecoverable(-4), isFalse);
        expect(MtProxyErrorInterpreter.isRecoverable(-6), isFalse);
      });
    });
  });

  group('MtProxyException', () {
    test('creates exception with message', () {
      const exception = MtProxyException('Test error');
      expect(exception.message, 'Test error');
      expect(exception.type, MtProxyErrorType.unknown);
    });

    test('creates exception with type and code', () {
      const exception = MtProxyException(
        'Error',
        type: MtProxyErrorType.initialization,
        code: -1,
      );
      expect(exception.type, MtProxyErrorType.initialization);
      expect(exception.errorCode, -1);
    });

    test('toString includes all details', () {
      const exception = MtProxyException(
        'Test error',
        type: MtProxyErrorType.runtime,
        code: -6,
        source: 'test',
      );
      final str = exception.toString();
      expect(str, contains('Test error'));
      expect(str, contains('источник: test'));
      expect(str, contains('код ошибки: -6'));
    });

    test('factory initialization creates correct type', () {
      const exception = MtProxyException.initialization('Init failed', code: -1);
      expect(exception.type, MtProxyErrorType.initialization);
      expect(exception.errorCode, -1);
    });

    test('factory configuration creates correct type', () {
      const exception = MtProxyException.configuration('Bad config', code: -5);
      expect(exception.type, MtProxyErrorType.configuration);
    });

    test('factory startup creates correct type', () {
      const exception = MtProxyException.startup('Start failed', code: -2);
      expect(exception.type, MtProxyErrorType.startup);
    });

    test('factory ffi creates correct type', () {
      const exception = MtProxyException.ffi('FFI error');
      expect(exception.type, MtProxyErrorType.ffi);
    });

    test('factory platform creates correct type', () {
      const exception = MtProxyException.platform('Platform error');
      expect(exception.type, MtProxyErrorType.platform);
    });
  });

  group('MtProxyResult', () {
    test('success result has isSuccess true', () {
      final result = MtProxyResult.success();
      expect(result.isSuccess, isTrue);
      expect(result.errorMessage, isNull);
    });

    test('failure result has isSuccess false', () {
      final result = MtProxyResult.failure(
        'Error message',
        type: MtProxyErrorType.runtime,
        code: -6,
      );
      expect(result.isSuccess, isFalse);
      expect(result.errorMessage, 'Error message');
      expect(result.errorType, MtProxyErrorType.runtime);
      expect(result.errorCode, -6);
    });

    test('throwIfError throws on failure', () {
      final result = MtProxyResult.failure('Error');
      expect(() => result.throwIfError(), throwsA(isA<MtProxyException>()));
    });

    test('throwIfError does nothing on success', () {
      final result = MtProxyResult.success();
      expect(result.throwIfError, returnsNormally);
    });

    test('onSuccess executes for success', () {
      var executed = false;
      MtProxyResult.success().onSuccess(() => executed = true);
      expect(executed, isTrue);
    });

    test('onSuccess does not execute for failure', () {
      var executed = false;
      MtProxyResult.failure('Error').onSuccess(() => executed = true);
      expect(executed, isFalse);
    });

    test('onFailure executes for failure', () {
      String? errorMessage;
      MtProxyResult.failure('Error').onFailure((msg) => errorMessage = msg);
      expect(errorMessage, 'Error');
    });

    test('onFailure does not execute for success', () {
      var executed = false;
      MtProxyResult.success().onFailure((_) => executed = true);
      expect(executed, isFalse);
    });
  });

  group('MtProxyErrorManager', () {
    test('starts with no errors', () {
      final manager = MtProxyErrorManager();
      expect(manager.errorCount, 0);
      expect(manager.lastError, isNull);
    });

    test('adds error to history', () {
      final manager = MtProxyErrorManager();
      final error = MtProxyException('Test error');
      manager.addError(error);
      
      expect(manager.errorCount, 1);
      expect(manager.lastError, error);
    });

    test('records native error', () {
      final manager = MtProxyErrorManager();
      manager.recordNativeError(-1, source: 'init');
      
      expect(manager.errorCount, 1);
      expect(manager.lastError?.errorCode, -1);
      expect(manager.lastError?.source, 'init');
    });

    test('limits history size', () {
      final manager = MtProxyErrorManager(maxHistorySize: 3);
      
      for (var i = 0; i < 5; i++) {
        manager.recordError('Error $i');
      }
      
      expect(manager.errorCount, 3);
      expect(manager.lastError?.message, 'Error 4');
    });

    test('clears history', () {
      final manager = MtProxyErrorManager();
      manager.recordError('Error 1');
      manager.recordError('Error 2');
      
      manager.clear();
      
      expect(manager.errorCount, 0);
      expect(manager.lastError, isNull);
    });

    test('filters errors by type', () {
      final manager = MtProxyErrorManager();
      manager.recordError(
        'Init error',
        type: MtProxyErrorType.initialization,
      );
      manager.recordError(
        'Config error',
        type: MtProxyErrorType.configuration,
      );
      manager.recordError(
        'Another init error',
        type: MtProxyErrorType.initialization,
      );
      
      final initErrors = manager.getErrorsByType(MtProxyErrorType.initialization);
      expect(initErrors.length, 2);
    });

    test('gets critical errors', () {
      final manager = MtProxyErrorManager();
      manager.recordNativeError(-1); // critical
      manager.recordNativeError(-2); // not critical
      manager.recordNativeError(-6); // critical
      
      final critical = manager.getCriticalErrors();
      expect(critical.length, 2);
    });

    test('gets error statistics', () {
      final manager = MtProxyErrorManager();
      manager.recordError(
        'Init',
        type: MtProxyErrorType.initialization,
      );
      manager.recordError(
        'Config',
        type: MtProxyErrorType.configuration,
      );
      manager.recordError(
        'Init 2',
        type: MtProxyErrorType.initialization,
      );
      
      final stats = manager.getErrorStatistics();
      expect(stats['initialization'], 2);
      expect(stats['configuration'], 1);
    });
  });
}
