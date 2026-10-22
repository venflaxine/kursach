#include "TextAnalyzer.h"

#include <QChar>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace TextSimilarity {
namespace {

const QChar WordSeparator(0x001F);

}

bool TextAnalyzer::isUnicodePunctuation(char32_t codePoint)
{
    switch (QChar::category(codePoint)) {
    case QChar::Punctuation_Connector:
    case QChar::Punctuation_Dash:
    case QChar::Punctuation_Open:
    case QChar::Punctuation_Close:
    case QChar::Punctuation_InitialQuote:
    case QChar::Punctuation_FinalQuote:
    case QChar::Punctuation_Other:
        return true;
    default:
        return false;
    }
}

QVector<QString> TextAnalyzer::tokenize(const QString &text, const Options &options)
{
    QString normalized = text;
    if (options.ignoreCase)
        normalized = normalized.toCaseFolded();

    if (options.mode == GramMode::Words) {
        QVector<QString> tokens;

        if (options.ignorePunctuation) {
            static const QRegularExpression wordPattern(
                QStringLiteral("[\\p{L}\\p{N}_]+"));
            auto matches = wordPattern.globalMatch(normalized);
            while (matches.hasNext())
                tokens.append(matches.next().captured(0));
        } else {
            const QStringList pieces = normalized.split(
                QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            tokens.reserve(pieces.size());
            for (const QString &piece : pieces)
                tokens.append(piece);
        }
        return tokens;
    }
    normalized.replace(QRegularExpression(QStringLiteral("\\s+")),
                       QStringLiteral(" "));

    const QVector<uint> codePoints = normalized.toUcs4();
    QVector<QString> tokens;
    tokens.reserve(codePoints.size());
    bool previousWasSpace = false;

    for (uint rawCodePoint : codePoints) {
        const char32_t codePoint = static_cast<char32_t>(rawCodePoint);
        if (options.ignorePunctuation && isUnicodePunctuation(codePoint))
            continue;

        const bool isSpace = QChar::isSpace(codePoint);
        if (isSpace) {
            if (previousWasSpace)
                continue;
            previousWasSpace = true;
        } else {
            previousWasSpace = false;
        }

        tokens.append(QString::fromUcs4(&codePoint, 1));
    }

    return tokens;
}

TextAnalyzer::FrequencyMap TextAnalyzer::buildNgrams(const QVector<QString> &tokens,
                                                       const Options &options)
{
    FrequencyMap frequencies;
    if (options.n <= 0 || tokens.size() < options.n)
        return frequencies;

    const qsizetype lastStart = tokens.size() - options.n;
    for (qsizetype start = 0; start <= lastStart; ++start) {
        QString gram;
        if (options.mode == GramMode::Words) {
            for (int offset = 0; offset < options.n; ++offset) {
                if (offset > 0)
                    gram.append(WordSeparator);
                gram.append(tokens.at(start + offset));
            }
        } else {
            for (int offset = 0; offset < options.n; ++offset)
                gram.append(tokens.at(start + offset));
        }
        ++frequencies[gram];
    }

    return frequencies;
}

int TextAnalyzer::sumFrequencies(const FrequencyMap &frequencies)
{
    int total = 0;
    for (auto it = frequencies.cbegin(); it != frequencies.cend(); ++it)
        total += it.value();
    return total;
}

QString TextAnalyzer::gramToDisplay(const QString &encodedGram, GramMode mode)
{
    if (mode == GramMode::Words) {
        QString display = encodedGram;
        display.replace(WordSeparator, QLatin1Char(' '));
        return display;
    }

    // Make spaces inside character N-grams visible in the result table.
    QString display = encodedGram;
    display.replace(QLatin1Char(' '), QStringLiteral("␠"));
    display.replace(QLatin1Char('\t'), QStringLiteral("⇥"));
    return display;
}

AnalysisResult TextAnalyzer::compare(const QString &textA,
                                     const QString &textB,
                                     const Options &options)
{
    AnalysisResult result;

    if (options.n < 1 || options.n > 10) {
        result.error = QStringLiteral("Размер N должен находиться в диапазоне от 1 до 10.");
        return result;
    }
    if (textA.trimmed().isEmpty()) {
        result.error = QStringLiteral("Первый текст пуст. Введите текст или загрузите файл.");
        return result;
    }
    if (textB.trimmed().isEmpty()) {
        result.error = QStringLiteral("Второй текст пуст. Введите текст или загрузите файл.");
        return result;
    }

    const QVector<QString> tokensA = tokenize(textA, options);
    const QVector<QString> tokensB = tokenize(textB, options);
    const FrequencyMap gramsA = buildNgrams(tokensA, options);
    const FrequencyMap gramsB = buildNgrams(tokensB, options);

    if (gramsA.isEmpty()) {
        result.error = QStringLiteral(
            "В первом тексте недостаточно элементов для N-грамм размера %1. Уменьшите N или добавьте текст.")
                           .arg(options.n);
        return result;
    }
    if (gramsB.isEmpty()) {
        result.error = QStringLiteral(
            "Во втором тексте недостаточно элементов для N-грамм размера %1. Уменьшите N или добавьте текст.")
                           .arg(options.n);
        return result;
    }

    result.totalNgramsA = sumFrequencies(gramsA);
    result.totalNgramsB = sumFrequencies(gramsB);
    result.uniqueNgramsA = gramsA.size();
    result.uniqueNgramsB = gramsB.size();

    double dotProduct = 0.0;
    double squaredNormA = 0.0;
    double squaredNormB = 0.0;

    for (auto it = gramsA.cbegin(); it != gramsA.cend(); ++it) {
        const double countA = it.value();
        squaredNormA += countA * countA;

        const auto foundB = gramsB.constFind(it.key());
        if (foundB != gramsB.cend()) {
            const double countB = foundB.value();
            dotProduct += countA * countB;

            ++result.commonUniqueNgrams;
            result.commonGrams.append(
                CommonGram{it.key(), it.value(), foundB.value()});
        }
    }

    for (auto it = gramsB.cbegin(); it != gramsB.cend(); ++it) {
        const double countB = it.value();
        squaredNormB += countB * countB;
    }

    const int unionSize = result.uniqueNgramsA + result.uniqueNgramsB
                          - result.commonUniqueNgrams;
    result.jaccard = unionSize > 0
                         ? static_cast<double>(result.commonUniqueNgrams) / unionSize
                         : 0.0;

    const double normProduct = std::sqrt(squaredNormA * squaredNormB);
    result.cosine = normProduct > 0.0 ? dotProduct / normProduct : 0.0;

    std::sort(result.commonGrams.begin(), result.commonGrams.end(),
              [](const CommonGram &left, const CommonGram &right) {
                  const int leftCombined = left.countA + left.countB;
                  const int rightCombined = right.countA + right.countB;
                  if (leftCombined != rightCombined)
                      return leftCombined > rightCombined;
                  return left.gram < right.gram;
              });

    result.ok = true;
    return result;
}

}
