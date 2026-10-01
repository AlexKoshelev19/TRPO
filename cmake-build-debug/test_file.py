from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, Doc
#импорт классов: segmenter - разбивает на токены
#Doc - хранит всю информацию об анализе
#news_embedding -
#NewsMorphTagger - определяет части речи и грамматические признаки
text = "Иван приехал в Москву, отдохнул и встретил старого друга." #исходный текст
from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, Doc
from stressonnx import stress

def analyze(input_file: str ):

    with open(input_file + '.txt', 'r', encoding='utf-8') as f:
        content = f.read()
    #создание объектов классов
    segmenter = Segmenter()
    emb = NewsEmbedding()
    morph_tagger = NewsMorphTagger(emb)

    doc = Doc(content) #контейнер для текста и результатов анализа

    # Разбиваем текст на токены
    doc.segment(segmenter)

    # Определяем части речи
    doc.tag_morph(morph_tagger)
    verbs = [token.text for token in doc.tokens if token.pos == 'VERB']
    for verb in verbs:
       verb = stress(verb, "ru")
       print(verb)
    return verbs

analyze('input')
