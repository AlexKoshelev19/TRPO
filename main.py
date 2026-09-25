from natasha import Segmenter, NewsEmbedding, NewsMorphTagger, Doc
#импорт классов: segmenter - разбивает на токены
#Doc - хранит всю информацию об анализе
#news_embedding -
#NewsMorphTagger - определяет части речи и грамматические признаки
text = "Иван приехал в Москву, отдохнул и встретил старого друга." #исходный текст

#создание объектов классов
segmenter = Segmenter()
emb = NewsEmbedding()
morph_tagger = NewsMorphTagger(emb)

doc = Doc(text) #контейнер для текста и результатов анализа

# Разбиваем текст на токены
doc.segment(segmenter)

# Определяем части речи
doc.tag_morph(morph_tagger)

# Выводим все глаголы
for token in doc.tokens:
    if token.pos == 'VERB':
        print(token.text)