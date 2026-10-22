#include "similarity_engine.h"

#include <QChar>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {

using FrequencyMap = QMap<QString, int>;

void appendCodePoint(QString &target, uint codePoint)
{
    // QString uses UTF-16. Build surrogate pairs explicitly for supplementary Unicode.
    if (codePoint <= 0xFFFFU) {
        target.append(QChar(static_cast<ushort>(codePoint)));
        return;
    }

    const uint adjusted = codePoint - 0x10000U;
    const ushort high = static_cast<ushort>(0xD800U + (adjusted >> 10U));
    const ushort low = static_cast<ushort>(0xDC00U + (adjusted & 0x3FFU));
    target.append(QChar(high));
    target.append(QChar(low));
}

bool isCombiningMark(uint codePoint)
{
    const QChar::Category category = QChar::category(static_cast<char32_t>(codePoint));
    return category == QChar::Mark_NonSpacing
        || category == QChar::Mark_SpacingCombining
        || category == QChar::Mark_Enclosing;
}

QVector<QString> tokenizeWords(const QString &text, bool caseInsensitive)
{
    const QString normalized = caseInsensitive ? text.toCaseFolded() : text;
    const QList<uint> codePoints = normalized.toUcs4();

    QVector<QString> words;
    words.reserve(codePoints.size() / 5 + 1);
    QString current;

    for (uint codePoint : codePoints) {
        const bool letterOrNumber = QChar::isLetterOrNumber(static_cast<char32_t>(codePoint));
        const bool mark = !current.isEmpty() && isCombiningMark(codePoint);

        if (letterOrNumber || mark) {
            appendCodePoint(current, codePoint);
        } else if (!current.isEmpty()) {
            words.push_back(current);
            current.clear();
        }
    }

    if (!current.isEmpty()) {
        words.push_back(current);
    }

    return words;
}

QVector<uint> normalizeCharacters(const QString &text,
                                  const SimilarityEngine::Options &options)
{
    const QString normalized = options.caseInsensitive ? text.toCaseFolded() : text;
    const QList<uint> codePoints = normalized.toUcs4();

    QVector<uint> characters;
    characters.reserve(codePoints.size());

    for (uint codePoint : codePoints) {
        if (options.ignoreWhitespace && QChar::isSpace(static_cast<char32_t>(codePoint))) {
            continue;
        }
        if (options.ignorePunctuation && QChar::isPunct(static_cast<char32_t>(codePoint))) {
            continue;
        }
        characters.push_back(codePoint);
    }

    return characters;
}

FrequencyMap makeWordNGrams(const QVector<QString> &words, int n)
{
    FrequencyMap frequencies;
    if (words.size() < n) {
        return frequencies;
    }

    const QChar separator(static_cast<char16_t>(0x001F));
    for (qsizetype i = 0; i <= words.size() - n; ++i) {
        QString gram;
        for (int j = 0; j < n; ++j) {
            if (j > 0) {
                gram.append(separator);
            }
            gram.append(words.at(i + j));
        }
        ++frequencies[gram];
    }
    return frequencies;
}

FrequencyMap makeCharacterNGrams(const QVector<uint> &characters, int n)
{
    FrequencyMap frequencies;
    if (characters.size() < n) {
        return frequencies;
    }

    for (qsizetype i = 0; i <= characters.size() - n; ++i) {
        QString gram;
        for (int j = 0; j < n; ++j) {
            appendCodePoint(gram, characters.at(i + j));
        }
        ++frequencies[gram];
    }
    return frequencies;
}

qsizetype totalOccurrences(const FrequencyMap &frequencies)
{
    qsizetype total = 0;
    for (auto it = frequencies.cbegin(); it != frequencies.cend(); ++it) {
        total += it.value();
    }
    return total;
}

double jaccardSimilarity(const FrequencyMap &a,
                         const FrequencyMap &b,
                         qsizetype sharedUniqueCount)
{
    const qsizetype unionCount = a.size() + b.size() - sharedUniqueCount;
    if (unionCount <= 0) {
        return 0.0;
    }
    return static_cast<double>(sharedUniqueCount) / static_cast<double>(unionCount);
}

double cosineSimilarity(const FrequencyMap &a, const FrequencyMap &b)
{
    double dotProduct = 0.0;
    double normA = 0.0;
    double normB = 0.0;

    for (auto it = a.cbegin(); it != a.cend(); ++it) {
        const double count = static_cast<double>(it.value());
        normA += count * count;
        const auto other = b.constFind(it.key());
        if (other != b.cend()) {
            dotProduct += count * static_cast<double>(other.value());
        }
    }

    for (auto it = b.cbegin(); it != b.cend(); ++it) {
        const double count = static_cast<double>(it.value());
        normB += count * count;
    }

    if (normA <= 0.0 || normB <= 0.0) {
        return 0.0;
    }

    const double score = dotProduct / (std::sqrt(normA) * std::sqrt(normB));
    return std::clamp(score, 0.0, 1.0);
}

} // namespace

SimilarityEngine::Result SimilarityEngine::compare(const QString &textA,
                                                     const QString &textB,
                                                     const Options &options)
{
    Result result;

    if (textA.trimmed().isEmpty()) {
        result.errorMessage = QStringLiteral("Первый текст пуст. Вставьте текст или загрузите файл.");
        return result;
    }
    if (textB.trimmed().isEmpty()) {
        result.errorMessage = QStringLiteral("Второй текст пуст. Вставьте текст или загрузите файл.");
        return result;
    }
    if (textA.size() > MaxTextLength || textB.size() > MaxTextLength) {
        result.errorMessage = QStringLiteral("Слишком большой текст. Максимум — %1 символов на каждый текст.")
                                  .arg(MaxTextLength);
        return result;
    }
    if (options.n < MinNGramSize || options.n > MaxNGramSize) {
        result.errorMessage = QStringLiteral("Размер N-граммы должен быть от %1 до %2.")
                                  .arg(MinNGramSize)
                                  .arg(MaxNGramSize);
        return result;
    }

    FrequencyMap frequenciesA;
    FrequencyMap frequenciesB;

    qsizetype availableA = 0;
    qsizetype availableB = 0;

    if (options.mode == NGramMode::Words) {
        const QVector<QString> wordsA = tokenizeWords(textA, options.caseInsensitive);
        const QVector<QString> wordsB = tokenizeWords(textB, options.caseInsensitive);
        availableA = wordsA.size();
        availableB = wordsB.size();

        if (availableA < options.n || availableB < options.n) {
            result.errorMessage = QStringLiteral(
                "Недостаточно слов для выбранного N=%1. После обработки найдено: в тексте A — %2, в тексте B — %3. "
                "Уменьшите N или используйте более длинные тексты.")
                                      .arg(options.n)
                                      .arg(availableA)
                                      .arg(availableB);
            return result;
        }

        frequenciesA = makeWordNGrams(wordsA, options.n);
        frequenciesB = makeWordNGrams(wordsB, options.n);
    } else {
        const QVector<uint> charactersA = normalizeCharacters(textA, options);
        const QVector<uint> charactersB = normalizeCharacters(textB, options);
        availableA = charactersA.size();
        availableB = charactersB.size();

        if (availableA < options.n || availableB < options.n) {
            result.errorMessage = QStringLiteral(
                "Недостаточно символов для выбранного N=%1. После обработки найдено: в тексте A — %2, в тексте B — %3. "
                "Измените параметры обработки или используйте более длинные тексты.")
                                      .arg(options.n)
                                      .arg(availableA)
                                      .arg(availableB);
            return result;
        }

        frequenciesA = makeCharacterNGrams(charactersA, options.n);
        frequenciesB = makeCharacterNGrams(charactersB, options.n);
    }

    result.totalNGramsA = totalOccurrences(frequenciesA);
    result.totalNGramsB = totalOccurrences(frequenciesB);
    result.uniqueNGramsA = frequenciesA.size();
    result.uniqueNGramsB = frequenciesB.size();

    for (auto it = frequenciesA.cbegin(); it != frequenciesA.cend(); ++it) {
        const auto other = frequenciesB.constFind(it.key());
        if (other == frequenciesB.cend()) {
            continue;
        }

        SharedNGram shared;
        shared.value = it.key();
        shared.countA = it.value();
        shared.countB = other.value();
        shared.sharedCount = std::min(shared.countA, shared.countB);
        result.commonNGrams.push_back(shared);
    }

    result.sharedUniqueNGrams = result.commonNGrams.size();

    if (options.metric == SimilarityMetric::Jaccard) {
        result.similarity = jaccardSimilarity(frequenciesA, frequenciesB,
                                              result.sharedUniqueNGrams);
    } else {
        result.similarity = cosineSimilarity(frequenciesA, frequenciesB);
    }

    std::sort(result.commonNGrams.begin(), result.commonNGrams.end(),
              [](const SharedNGram &left, const SharedNGram &right) {
                  if (left.sharedCount != right.sharedCount) {
                      return left.sharedCount > right.sharedCount;
                  }
                  const int leftTotal = left.countA + left.countB;
                  const int rightTotal = right.countA + right.countB;
                  if (leftTotal != rightTotal) {
                      return leftTotal > rightTotal;
                  }
                  return left.value < right.value;
              });

    result.success = true;
    return result;
}

QString SimilarityEngine::displayNGram(const QString &value, NGramMode mode)
{
    QString display = value;
    if (mode == NGramMode::Words) {
        display.replace(QChar(static_cast<char16_t>(0x001F)), QLatin1Char(' '));
        return display;
    }

    display.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    display.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    display.replace(QStringLiteral("\t"), QStringLiteral("\\t"));
    display.replace(QLatin1Char(' '), QChar(static_cast<char16_t>(0x2420))); // Visible space: ␠
    return display;
}
