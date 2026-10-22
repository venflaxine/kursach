#pragma once // Защищает заголовочный файл от повторного подключения

// Библиотеки Qt
#include <QMap>     // Контейнер для хранения пар «ключ — значение», здесь используется для частот N-грамм
#include <QString>  // Строка Qt для хранения и обработки текста
#include <QVector>  // Контейнер Qt для хранения последовательности элементов

namespace TextSimilarity {

// Определяет способ разбиения текста на N-граммы
enum class GramMode {
    Words,       // N-граммы из слов
    Characters  // N-граммы из символов
};

// Настройки анализа текста
struct Options {
    int n = 2;                         // Количество слов или символов в одной N-грамме
    GramMode mode = GramMode::Words;   // Выбранный режим анализа
    bool ignoreCase = true;            // Игнорировать различия между заглавными и строчными буквами
    bool ignorePunctuation = true;     // Игнорировать знаки пунктуации
};

// Информация об общей N-грамме в двух текстах
struct CommonGram {
    QString gram;  // Содержимое N-граммы
    int countA = 0; // Сколько раз N-грамма встречается в первом тексте
    int countB = 0; // Сколько раз N-грамма встречается во втором тексте
};

// Результаты сравнения двух текстов
struct AnalysisResult {
    bool ok = false;   // Показывает, завершился ли анализ успешно
    QString error;     // Сообщение об ошибке, если анализ не удался

    double jaccard = 0.0; // Коэффициент сходства Жаккара
    double cosine = 0.0;  // Косинусное сходство текстов

    int totalNgramsA = 0;       // Общее количество N-грамм в первом тексте
    int totalNgramsB = 0;       // Общее количество N-грамм во втором тексте
    int uniqueNgramsA = 0;      // Количество уникальных N-грамм первого текста
    int uniqueNgramsB = 0;      // Количество уникальных N-грамм второго текста
    int commonUniqueNgrams = 0; // Количество общих уникальных N-грамм

    QVector<CommonGram> commonGrams; // Список общих N-грамм и частоты их появления
};

// Класс выполняет анализ и сравнение текстов
class TextAnalyzer final {
public:
    // Сравнивает два текста с заданными настройками
    static AnalysisResult compare(const QString &textA,
                                  const QString &textB,
                                  const Options &options);

    // Преобразует внутреннее представление N-граммы в читаемый вид
    static QString gramToDisplay(const QString &encodedGram, GramMode mode);

private:
    // Тип для хранения N-грамм и количества их появлений
    using FrequencyMap = QMap<QString, int>;

    // Разбивает текст на слова или символы с учётом настроек
    static QVector<QString> tokenize(const QString &text, const Options &options);

    // Строит N-граммы и подсчитывает частоту каждой из них
    static FrequencyMap buildNgrams(const QVector<QString> &tokens,
                                    const Options &options);

    // Считает общее количество появлений всех N-грамм
    static int sumFrequencies(const FrequencyMap &frequencies);

    // Проверяет, является ли Unicode-символ знаком пунктуации
    static bool isUnicodePunctuation(char32_t codePoint);
};

}