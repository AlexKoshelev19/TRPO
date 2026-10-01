import os
import warnings
from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, Doc
from ruaccent import RUAccent


# Инициализируем accentizer один раз при запуске скрипта
accentizer = RUAccent()
accentizer.load(omograph_model_size='tiny', use_dictionary=True)


def analyze(input_file: str):
    # 1. Читаем исходный текст
    file_path = input_file + '.txt'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 2. Расставляем ударения во ВСЕМ тексте сразу (так нейросеть учтет контекст)
    accented_content = accentizer.process_all(content)

    # 3. Настраиваем Natasha для поиска глаголов
    segmenter = Segmenter()
    emb = NewsEmbedding()
    morph_tagger = NewsMorphTagger(emb)

    # Передаем в Doc уже текст с ударениями
    doc = Doc(accented_content)
    doc.segment(segmenter)
    doc.tag_morph(morph_tagger)

    # 4. Собираем глаголы (они уже будут содержать знаки ударения)
    accented_verbs = [token.text for token in doc.tokens if token.pos == 'VERB']

    print("Найденные глаголы с ударениями:")
    print(accented_verbs)

    return accented_verbs


# Вызов функции (убедитесь, что файл test2.txt лежит рядом со скриптом)
if __name__ == "__main__":
    # Для теста можно сначала создать файл, если его нет
    if not os.path.exists('test2.txt'):
        with open('test2.txt', 'w', encoding='utf-8') as f:
            f.write("Иван приехал в Москву, отдохнул и встретил старого друга.")

    analyze('test2')
