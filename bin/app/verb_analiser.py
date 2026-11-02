import sys
import os
from pathlib import Path

current_dir = os.path.dirname(os.path.abspath(__file__))

# настройка путей для библиотек Python
lib_path = os.path.join(current_dir, "python_runtime", "lib")
if lib_path not in sys.path:
    sys.path.insert(0, lib_path)

# создание локальной папки для моделей (если её нет)
models_cache_dir = os.path.join(current_dir, "models_cache")
os.makedirs(models_cache_dir, exist_ok=True)

# перенаправление кэша загрузок в локальную папку
os.environ["HF_HOME"] = models_cache_dir
os.environ["XDG_CACHE_HOME"] = models_cache_dir
os.environ["HF_ENDPOINT"] = "https://hf-mirror.com"

# проверяем, есть ли внутри папки models_cache какие-либо файлы/папки.
has_cached_models = any(os.scandir(models_cache_dir))

import re
import pymorphy3
# импортируем модель natasha для выявления глаголов
from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, NewsSyntaxParser, Doc
# Импортируем функцию ударения из выбранной библиотеки
from stressonnx import stress

if has_cached_models:
    # если файлы - блокируем сетевые запросы Hugging Face
    os.environ["HF_HUB_OFFLINE"] = "1"
    os.environ["TRANSFORMERS_OFFLINE"] = "1"
else:
    # оставляем доступ в сеть для первоначального скачивания (на этапе разработки)
    pass

ACCENT_CHAR = '\u0301'  # Юникод-символ ударения, который ставит stressonnx

segmenter = Segmenter()
emb = NewsEmbedding()
morph_tagger = NewsMorphTagger(emb)
syntax_tagger = NewsSyntaxParser(emb)
morph_analyzer = pymorphy3.MorphAnalyzer()

def is_true_verb(word: str) -> bool:
    parses = morph_analyzer.parse(word)
    return any(p.tag.POS in ('VERB', 'INFN') for p in parses)

def analyze(input_file: str) -> dict:
    with open(input_file, 'r', encoding='utf-8') as f:
        text = f.read()

    # 1. Нормализация дефисных побудительных частиц (-ка, -то),
    # чтобы токенизатор не сбоил на словах вроде "Скажи-ка" -> "Скажи ка"
    normalized_text = re.sub(r'(\b\w+)-ка\b', r'\1 ка', text, flags=re.IGNORECASE)
    normalized_text = re.sub(r'(\b\w+)-то\b', r'\1 то', text, flags=re.IGNORECASE)

    doc = Doc(normalized_text)

    # 2. Разбор структуры
    doc.segment(segmenter)
    doc.tag_morph(morph_tagger)
    doc.parse_syntax(syntax_tagger)

    # Собираем ID слов, которые управляют актантами или обстоятельствами
    heads_with_arguments = {
        token.head_id for token in doc.tokens
        if token.rel in ("nsubj", "obj", "iobj", "obl", "advmod", "vocative")
    }

    # Предикативные синтаксические роли в дереве зависимостей
    PREDICATE_ROLES = {
        "root", "xcomp", "ccomp", "conj", "advcl", 
        "parataxis", "cop", "aux", "discourse"
    }

    # Роли, гарантированно указывающие на то, что слово выступает как существительное/прилагательное
    NOMINAL_ROLES = {"nsubj", "obj", "iobj", "nmod", "amod", "appos"}

    verbs = {}

    for token in doc.tokens:
        clean_text = token.text.lower()

        # Игнорируем знаки препинания и небуквенные токены
        if not clean_text.isalpha():
            continue

        feats = token.feats or {}
        pos = token.pos

        # Рассматриваем только предикативные части речи Natasha
        if pos in ('VERB', 'AUX'):
            # Критерий 1: Повелительное наклонение (Скажи, постой, умремте)
            is_imperative = feats.get("Mood") == "Imp"

            # Критерий 2: Инфинитив (изорвать, разгуляться, видать, колоть)
            is_infinitive = feats.get("VerbForm") == "Inf"

            # Критерий 3: Предикативный узел в синтаксическом дереве
            is_predicate = token.rel in PREDICATE_ROLES

            # Критерий 4: Управляет зависимыми членами предложения
            controls_args = token.id in heads_with_arguments

            # Проверка на ложноположительную именную позицию
            is_nominal = token.rel in NOMINAL_ROLES

            # Слово проходит синтаксический порог, если:
            # - это императив или инфинитив (они не требуют подлежащего)
            # - или является предикатом / управляет актантами, не будучи номинативным членом
            if (is_imperative or is_infinitive or is_predicate or controls_args) and not is_nominal:
                if is_true_verb(token.text):
                    verb = token.text.lower()
                    indexes =  verbs.get(verb) if verbs.get(verb) != None else []
                    indexes.append(int(token.id.split('_')[0]))
                    verbs[verb] = indexes
                    #print(verb)

    for verb in list(verbs.keys()):
        # Передаем слово и маркер языка "ru"
        word = stress(verb, "ru")

        # Ищем символ ударения (он стоит сразу ПОСЛЕ ударной гласной)
        indexes = verbs.pop(verb)
        verbs[word] = indexes

    in_path = Path(input_file)

    # Каталог для отредактированных файлов
    output_dir = Path("edited")
    output_dir.mkdir(parents=True, exist_ok=True)  # Автоматически создаёт папку, если её нет

    # in_path.stem берет только имя файла без расширения: "стих.txt" -> "стих"
    output_file = output_dir / f"{in_path.stem}_edited.txt" 
    with open(output_file, "w", encoding="utf-8") as f:
        for index, sentence in enumerate(doc.sents, start=1):
            sent_text = sentence.text.strip()

            # Проверка: если текст уже начинается с формата "(число) ", то не добавляем нумерацию повторно
            if re.match(r'^\(\d+\)\s', sent_text):
                f.write(f"{sent_text}\n")
            else:
                f.write(f"({index}) {sent_text}\n")
    return verbs