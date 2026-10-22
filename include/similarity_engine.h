#pragma once // Защищает файл от повторного подключения

// Библиотеки Qt для хранения данных и работы со строками
#include <QMap>     // Контейнер для хранения пар ключ-значение
#include <QString>  // Строки
#include <QVector>  // Динамический массив

    // Класс, который отвечает за сравнение текстов
    class SimilarityEngine final
{
public:
    // Ограничения на размер текста и размер N-граммы
    static constexpr qsizetype MaxTextLength = 200'000;
    static constexpr int MinNGramSize = 1;
    static constexpr int MaxNGramSize = 10;

    // Способ разбиения текста на N-граммы
    enum class NGramMode {
        Words,      // По словам
        Characters // По символам
    };

    // Метрика, по которой оценивается сходство текстов
    enum class SimilarityMetric {
        Jaccard, // Коэффициент Жаккара
        Cosine   // Косинусное сходство
    };

    // Настройки сравнения текстов
    struct Options {
        int n = 3; // Размер N-граммы
        NGramMode mode = NGramMode::Words; // Режим сравнения
        SimilarityMetric metric = SimilarityMetric::Jaccard; // Метрика сходства
        bool caseInsensitive = true; // Не учитывать регистр букв
        bool ignorePunctuation = true; // Игнорировать знаки препинания
        bool ignoreWhitespace = true; // Игнорировать пробельные символы
    };

    // Информация об общей N-грамме в двух текстах
    struct SharedNGram {
        QString value; // Сама N-грамма
        int countA = 0; // Сколько раз встречается в тексте A
        int countB = 0; // Сколько раз встречается в тексте B
        int sharedCount = 0; // Число общих вхождений
    };

    // Результат сравнения двух текстов
    struct Result {
        bool success = false; // Успешно ли завершён анализ
        QString errorMessage; // Сообщение об ошибке, если она возникла
        double similarity = 0.0; // Итоговая оценка сходства
        qsizetype totalNGramsA = 0; // Всего N-грамм в тексте A
        qsizetype totalNGramsB = 0; // Всего N-грамм в тексте B
        qsizetype uniqueNGramsA = 0; // Уникальных N-грамм в тексте A
        qsizetype uniqueNGramsB = 0; // Уникальных N-грамм в тексте B
        qsizetype sharedUniqueNGrams = 0; // Количество общих уникальных N-грамм
        QVector<SharedNGram> commonNGrams; // Список совпадений
    };

    // Сравнивает два текста с учётом выбранных настроек
    static Result compare(const QString &textA,
                          const QString &textB,
                          const Options &options);

    // Подготавливает N-грамму для отображения в интерфейсе
    static QString displayNGram(const QString &value, NGramMode mode);
};
