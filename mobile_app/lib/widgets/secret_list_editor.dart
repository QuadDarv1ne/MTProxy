import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// Виджет для редактирования списка секретных ключей
class SecretListEditor extends StatefulWidget {
  final List<String> secrets;
  final Function(List<String>) onChanged;

  const SecretListEditor({
    super.key,
    required this.secrets,
    required this.onChanged,
  });

  @override
  State<SecretListEditor> createState() => _SecretListEditorState();
}

class _SecretListEditorState extends State<SecretListEditor> {
  final Map<int, TextEditingController> _controllers = {};
  final Map<int, bool> _obscureText = {};

  @override
  void initState() {
    super.initState();
    _initControllers();
  }

  void _initControllers() {
    for (var i = 0; i < widget.secrets.length; i++) {
      if (!_controllers.containsKey(i)) {
        _controllers[i] = TextEditingController(text: widget.secrets[i]);
        _obscureText[i] = true;
      }
    }
  }

  @override
  void didUpdateWidget(SecretListEditor oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.secrets.length != oldWidget.secrets.length) {
      _initControllers();
    }
  }

  @override
  void dispose() {
    for (final controller in _controllers.values) {
      controller.dispose();
    }
    super.dispose();
  }

  void _notifyChanged() {
    final updatedSecrets = <String>[];
    for (var i = 0; i < widget.secrets.length; i++) {
      updatedSecrets.add(_controllers[i]?.text ?? widget.secrets[i]);
    }
    widget.onChanged(updatedSecrets);
  }

  void _generateSecret(int index) {
    final random = DateTime.now().millisecondsSinceEpoch;
    final secret = _generateHexSecret();
    setState(() {
      _controllers[index]?.text = secret;
      _notifyChanged();
    });
  }

  String _generateHexSecret() {
    final random = DateTime.now().millisecondsSinceEpoch;
    return List.generate(32, (i) {
      return ((random + i * 17) % 256).toRadixString(16).padLeft(2, '0');
    }).join();
  }

  void _pasteFromClipboard(int index) async {
    final clipboard = await Clipboard.getData('text/plain');
    if (clipboard?.text != null) {
      setState(() {
        _controllers[index]?.text = clipboard!.text!;
        _notifyChanged();
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: widget.secrets.asMap().entries.map((entry) {
        final index = entry.key;
        final secret = entry.value;
        
        return Padding(
          padding: const EdgeInsets.only(bottom: 8),
          child: Row(
            children: [
              Expanded(
                child: TextFormField(
                  controller: _controllers[index],
                  obscureText: _obscureText[index] ?? true,
                  decoration: InputDecoration(
                    hintText: 'Введите hex-ключ (64 символа)',
                    prefixIcon: const Icon(Icons.key),
                    suffixIcon: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        IconButton(
                          icon: Icon(
                            _obscureText[index] ?? true
                                ? Icons.visibility
                                : Icons.visibility_off,
                          ),
                          onPressed: () {
                            setState(() {
                              _obscureText[index] = !(_obscureText[index] ?? true);
                            });
                          },
                          tooltip: _obscureText[index] ?? true
                              ? 'Показать'
                              : 'Скрыть',
                        ),
                        IconButton(
                          icon: const Icon(Icons.refresh),
                          onPressed: () => _generateSecret(index),
                          tooltip: 'Сгенерировать',
                        ),
                        IconButton(
                          icon: const Icon(Icons.content_paste),
                          onPressed: () => _pasteFromClipboard(index),
                          tooltip: 'Вставить',
                        ),
                      ],
                    ),
                  ),
                  validator: (value) {
                    if (value == null || value.isEmpty) return null;
                    if (value.length != 64) {
                      return 'Должен быть 64 hex-символа (32 байта)';
                    }
                    if (!RegExp(r'^[0-9a-fA-F]+$').hasMatch(value)) {
                      return 'Только hex-символы (0-9, a-f)';
                    }
                    return null;
                  },
                  onChanged: (_) => _notifyChanged(),
                ),
              ),
              if (widget.secrets.length > 1)
                IconButton(
                  icon: const Icon(Icons.delete_outline, color: Colors.red),
                  onPressed: () {
                    setState(() {
                      widget.secrets.removeAt(index);
                      _notifyChanged();
                    });
                  },
                  tooltip: 'Удалить ключ',
                ),
            ],
          ),
        );
      }).toList(),
    );
  }
}
