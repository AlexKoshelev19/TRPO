import sys
import os

# Добавляем путь к python_runtime/lib
current_dir = os.path.dirname(os.path.abspath(__file__))
lib_path = os.path.join(current_dir, "python_runtime", "lib")
if lib_path not in sys.path:
    sys.path.insert(0, lib_path)

# Обход блокировки Hugging Face (перенаправление на рабочее зеркало)
os.environ["HF_ENDPOINT"] = "https://hf-mirror.com"

from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, Doc



def analyze(input_file: str):
    # Инициализация ruaccent (без CUDA, если нет видеокарты Nvidia)
    accentizer = RUAccent()
    accentizer.load()

    # Считываем файл
    with open(input_file + '.txt', 'r', encoding='utf-8') as f:
        content = f.read()

    # Создание объектов классов Natasha
    segmenter = Segmenter()
    emb = NewsEmbedding()
    morph_tagger = NewsMorphTagger(emb)

    doc = Doc(content)

    # Анализ текста с помощью Natasha
    doc.segment(segmenter)
    doc.tag_morph(morph_tagger)

    # Собираем глаголы
    verbs = [token.text for token in doc.tokens if token.pos == 'VERB']

    for verb in verbs:
        # ruaccent возвращает строку со знаком '+' ПЕРЕД ударной гласной
        processed = accentizer.process_all(verb)

        # Если вернулся список (в некоторых версиях), берем первый элемент
        if isinstance(processed, list):
            processed = processed[0] if processed else verb

        # Ищем плюс, который ставит ruaccent
        accent_index = processed.find('+')

        if accent_index != -1:
            # Буква после плюса должна стать заглавной
            # Удаляем сам плюс из строки, а букву, которая шла за ним, делаем большой
            clean_word = processed.replace('+', '')

            # Индекс ударной буквы в строке без плюса совпадает со старым индексом самого плюса
            idx = accent_index
            if idx < len(clean_word):
                new_word = clean_word[:idx] + clean_word[idx].upper() + clean_word[idx + 1:]
                print(f"Глагол: {verb} -> {new_word}")
            else:
                print(f"Глагол: {verb} -> {clean_word}")
        else:
            print(f"Ударение в слове '{verb}' не найдено")

    return verbs


if __name__ == "__main__":
    # Убедитесь, что рядом лежит файл test.txt
    analyze("test")
