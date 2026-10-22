#include "similarity_engine.h"

#include <QtTest/QtTest>

#include <cmath>

class SimilarityEngineTests final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsEmptyText();
    void rejectsInvalidN();
    void rejectsTooLongText();
    void rejectsInsufficientNGrams();
    void identicalTextsHaveFullSimilarity();
    void comparisonCanIgnoreCase();
    void comparisonCanRespectCase();
    void jaccardUsesUniqueNGrams();
    void cosineUsesNGramFrequencies();
    void characterModeSupportsRussian();
    void punctuationCanBeIgnoredInCharacterMode();
    void whitespaceCanBeIgnoredInCharacterMode();
    void reportsCommonNGramsInFrequencyOrder();
};

void SimilarityEngineTests::rejectsEmptyText()
{
    SimilarityEngine::Options options;
    auto result = SimilarityEngine::compare(QString(), QStringLiteral("здесь есть слова"), options);
    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());

    result = SimilarityEngine::compare(QStringLiteral("   \n\t"), QStringLiteral("здесь есть слова"), options);
    QVERIFY(!result.success);
}

void SimilarityEngineTests::rejectsInvalidN()
{
    SimilarityEngine::Options options;
    options.n = 0;
    auto result = SimilarityEngine::compare(QStringLiteral("один два"), QStringLiteral("один два"), options);
    QVERIFY(!result.success);
    QVERIFY(result.errorMessage.contains(QStringLiteral("от 1 до 10")));

    options.n = 11;
    result = SimilarityEngine::compare(QStringLiteral("один два"), QStringLiteral("один два"), options);
    QVERIFY(!result.success);
}

void SimilarityEngineTests::rejectsTooLongText()
{
    SimilarityEngine::Options options;
    options.n = 1;
    const QString tooLong(SimilarityEngine::MaxTextLength + 1, QLatin1Char('x'));
    const auto result = SimilarityEngine::compare(tooLong, QStringLiteral("короткий текст"), options);
    QVERIFY(!result.success);
    QVERIFY(result.errorMessage.contains(QStringLiteral("Слишком большой текст")));
}

void SimilarityEngineTests::rejectsInsufficientNGrams()
{
    SimilarityEngine::Options options;
    options.n = 2;
    options.mode = SimilarityEngine::NGramMode::Words;
    const auto result = SimilarityEngine::compare(QStringLiteral("один"), QStringLiteral("два"), options);
    QVERIFY(!result.success);
    QVERIFY(result.errorMessage.contains(QStringLiteral("Недостаточно слов")));
}

void SimilarityEngineTests::identicalTextsHaveFullSimilarity()
{
    SimilarityEngine::Options options;
    options.n = 2;
    const QString text = QStringLiteral("Локальный анализ текста проверяет общие последовательности слов.");
    const auto result = SimilarityEngine::compare(text, text, options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(result.similarity, 1.0);
    QCOMPARE(result.uniqueNGramsA, result.uniqueNGramsB);
    QCOMPARE(result.sharedUniqueNGrams, result.uniqueNGramsA);
}

void SimilarityEngineTests::comparisonCanIgnoreCase()
{
    SimilarityEngine::Options options;
    options.n = 1;
    options.caseInsensitive = true;
    const auto result = SimilarityEngine::compare(QStringLiteral("Привет МИР"), QStringLiteral("привет мир"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(result.similarity, 1.0);
}

void SimilarityEngineTests::comparisonCanRespectCase()
{
    SimilarityEngine::Options options;
    options.n = 1;
    options.caseInsensitive = false;
    const auto result = SimilarityEngine::compare(QStringLiteral("Привет"), QStringLiteral("привет"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(result.similarity, 0.0);
}

void SimilarityEngineTests::jaccardUsesUniqueNGrams()
{
    SimilarityEngine::Options options;
    options.n = 2;
    options.metric = SimilarityEngine::SimilarityMetric::Jaccard;
    const auto result = SimilarityEngine::compare(QStringLiteral("a b c"), QStringLiteral("a b d"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QVERIFY(std::abs(result.similarity - (1.0 / 3.0)) < 0.000001);
    QCOMPARE(result.sharedUniqueNGrams, qsizetype(1));
    QCOMPARE(result.uniqueNGramsA, qsizetype(2));
    QCOMPARE(result.uniqueNGramsB, qsizetype(2));
}

void SimilarityEngineTests::cosineUsesNGramFrequencies()
{
    SimilarityEngine::Options options;
    options.n = 1;
    options.metric = SimilarityEngine::SimilarityMetric::Cosine;
    const auto result = SimilarityEngine::compare(QStringLiteral("a a a b"), QStringLiteral("a a b b"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    const double expected = 8.0 / std::sqrt(80.0);
    QVERIFY(std::abs(result.similarity - expected) < 0.000001);
}

void SimilarityEngineTests::characterModeSupportsRussian()
{
    SimilarityEngine::Options options;
    options.n = 2;
    options.mode = SimilarityEngine::NGramMode::Characters;
    const auto result = SimilarityEngine::compare(QStringLiteral("мама мыла раму"), QStringLiteral("мама мыла раму"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(result.similarity, 1.0);
}

void SimilarityEngineTests::punctuationCanBeIgnoredInCharacterMode()
{
    SimilarityEngine::Options options;
    options.n = 2;
    options.mode = SimilarityEngine::NGramMode::Characters;
    options.ignorePunctuation = true;
    options.ignoreWhitespace = true;
    const auto result = SimilarityEngine::compare(QStringLiteral("аб,вг"), QStringLiteral("абвг"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(result.similarity, 1.0);
}

void SimilarityEngineTests::whitespaceCanBeIgnoredInCharacterMode()
{
    SimilarityEngine::Options options;
    options.n = 2;
    options.mode = SimilarityEngine::NGramMode::Characters;
    options.ignoreWhitespace = true;
    options.ignorePunctuation = true;
    const auto result = SimilarityEngine::compare(QStringLiteral("аб вг"), QStringLiteral("абвг"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(result.similarity, 1.0);
}

void SimilarityEngineTests::reportsCommonNGramsInFrequencyOrder()
{
    SimilarityEngine::Options options;
    options.n = 1;
    options.mode = SimilarityEngine::NGramMode::Words;
    const auto result = SimilarityEngine::compare(QStringLiteral("кот кот пёс"), QStringLiteral("кот кот кот пёс"), options);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QVERIFY(result.commonNGrams.size() >= 2);
    QCOMPARE(result.commonNGrams.first().value, QStringLiteral("кот"));
    QCOMPARE(result.commonNGrams.first().sharedCount, 2);
}

QTEST_GUILESS_MAIN(SimilarityEngineTests)
#include "test_similarity_engine.moc"
