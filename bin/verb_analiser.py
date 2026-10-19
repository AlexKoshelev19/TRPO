import sys
import os
import re
# импортируем модель natasha для выявления глаголов
from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, NewsSyntaxParser, Doc
# Импортируем функцию ударения из выбранной библиотеки
from stressonnx import stress

current_dir = os.path.dirname(os.path.abspath(__file__))

# настройка путей для библиотек Python
lib_path = os.path.join(current_dir, "python_runtime", "lib")
if lib_path not in sys.path:
    sys.path.insert(0, lib_path)

# 2. создание локальной папки для моделей (если её нет)
models_cache_dir = os.path.join(current_dir, "models_cache")
os.makedirs(models_cache_dir, exist_ok=True)

# перенаправление кэша загрузок в локальную папку
os.environ["HF_HOME"] = models_cache_dir
os.environ["XDG_CACHE_HOME"] = models_cache_dir
os.environ["HF_ENDPOINT"] = "https://hf-mirror.com"

# 4. проверяем, есть ли внутри папки models_cache какие-либо файлы/папки.
has_cached_models = any(os.scandir(models_cache_dir))

if has_cached_models:
    # если файлы - блокируем сетевые запросы Hugging Face
    os.environ["HF_HUB_OFFLINE"] = "1"
    os.environ["TRANSFORMERS_OFFLINE"] = "1"
else:
    # оставляем доступ в сеть для первоначального скачивания (на этапе разработки)
    pass

ACCENT_CHAR = '\u0301'  # Юникод-символ ударения, который ставит stressonnx

def analyze(input_file: str):
    with open(input_file, 'r', encoding='utf-8') as f:
        text = f.read()

    # Инициализация модулей Natasha
    segmenter = Segmenter()
    emb = NewsEmbedding()
    morph_tagger = NewsMorphTagger(emb)
    syntax_tagger = NewsSyntaxParser(emb)

    doc = Doc(text)

    # Разбор предложения на токены и части речи
    doc.segment(segmenter)

    # моррфологический разбор (присваивает token.pos)
    doc.tag_morph(morph_tagger)

    # синтаксический разбор (присваивает token.rel и token.head_id для выявления зависемостей между словами)
    doc.parse_syntax(syntax_tagger)

    # ищем слова, у которых есть свои исполнители или зависимые предметы
    heads_with_arguments = set()
    for token in doc.tokens:
        if token.rel in ("nsubj", "obj", "iobj", "obl"):
            heads_with_arguments.add(token.head_id)

    verbs = dict()
    # Выбираем глаголы
    for token in doc.tokens:
        if token.pos in ('VERB', 'AUX'):
            # Критерий А: токен сам является предикативным узлом
            is_predicate_node = token.rel in ("root", "xcomp", "ccomp", "conj", "advcl")

            # Критерий Б: токен управляет хотя бы одним глагольным зависимым
            controls_arguments = token.id in heads_with_arguments

            # Критерий В: отсекаем явно неглагольные роли (подлежащее, определение, приложение)
            is_nominal_role = token.rel in ("nsubj", "obj", "iobj", "nmod", "amod", "appos")

            if (is_predicate_node or controls_arguments) and not is_nominal_role:
                verb = token.text.lower()
                indexes =  verbs.get(verb) if verbs.get(verb) != None else []
                indexes.append(int(token.id.split('_')[0]))
                verbs[verb] = indexes
                #print(verb)

    # поиск ударений в глаголах
    for verb in list(verbs.keys()):
        word = stress(verb, "ru")
        indexes = verbs.pop(verb)
        verbs[word] = indexes


    with open(input_file, "w", encoding="utf-8") as f:
        for index, sentence in enumerate(doc.sents, start=1):
            sent_text = sentence.text.strip()

            # Проверка: если текст уже начинается с формата "(число) ", то не добавляем нумерацию повторно
            if re.match(r'^\(\d+\)\s', sent_text):
                f.write(f"{sent_text}\n")
            else:
                f.write(f"({index}) {sent_text}\n")
    return verbs