import sys
import os

current_dir = os.path.dirname(os.path.abspath(__file__))

# 1. Настройка путей для библиотек Python
lib_path = os.path.join(current_dir, "python_runtime", "lib")
if lib_path not in sys.path:
    sys.path.insert(0, lib_path)

# 2. Создаем локальную папку для моделей (если её нет)
models_cache_dir = os.path.join(current_dir, "models_cache")
os.makedirs(models_cache_dir, exist_ok=True)

# 3. Перенаправляем кэш загрузок в локальную папку
os.environ["HF_HOME"] = models_cache_dir
os.environ["XDG_CACHE_HOME"] = models_cache_dir
os.environ["HF_ENDPOINT"] = "https://hf-mirror.com"

# 4. ПРОВЕРКА НА БЛОКИРОВКУ СКАЧИВАНИЯ
# Проверяем, есть ли внутри папки models_cache какие-либо файлы/папки.
# Функция scandir работает очень быстро и не загружает весь список в память.
has_cached_models = any(os.scandir(models_cache_dir))

if has_cached_models:
    # Если файлы есть, жестко блокируем любые сетевые запросы Hugging Face
    os.environ["HF_HUB_OFFLINE"] = "1"
    os.environ["TRANSFORMERS_OFFLINE"] = "1"
else:
    # Оставляем доступ в сеть для первоначального скачивания (на этапе разработки)
    pass

from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, Doc
# Импортируем функцию ударения из выбранной библиотеки
from stressonnx import stress

ACCENT_CHAR = '\u0301'  # Юникод-символ ударения, который ставит stressonnx


def analyze(input_file: str):
    with open(input_file + '.txt', 'r', encoding='utf-8') as f:
        content = f.read()

    # Инициализация модулей Natasha
    segmenter = Segmenter()
    emb = NewsEmbedding()
    morph_tagger = NewsMorphTagger(emb)
    doc = Doc(content)

    # Разбор текста на токены и части речи
    doc.segment(segmenter)
    doc.tag_morph(morph_tagger)

    # Выбираем глаголы
    verbs = [token.text for token in doc.tokens if token.pos == 'VERB']


    # Используем enumerate, чтобы получить и сам глагол (verb), и его индекс (i) в массиве
    for i, verb in enumerate(verbs):
        # Передаем слово и маркер языка "ru"
        word = stress(verb, "ru")

        # Ищем символ ударения (он стоит сразу ПОСЛЕ ударной гласной)
        accent_index = word.find(ACCENT_CHAR)

        if accent_index != -1:
            # Убираем знак ударения, чтобы работать с буквами
            clean_word = word.replace(ACCENT_CHAR, '')

            # Ударная буква находится на позицию раньше, чем стоял символ ударения
            idx = accent_index - 1

            if 0 <= idx < len(clean_word):
                new_word = clean_word[:idx] + clean_word[idx].upper() + clean_word[idx + 1:]
                # ПЕРЕЗАПИСЫВАЕМ элемент в массиве verbs под текущим индексом
                verbs[i] = new_word

            else:
                verbs[i] = clean_word
        else:

            # Если ударение не нашли, оставляем слово без изменений (или обрабатываем иначе)
            verbs[i] = verb

    # Теперь здесь вернется массив уже с измененными строками!
    return verbs



