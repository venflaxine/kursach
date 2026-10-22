#pragma once

#include <QMap>
#include <QString>
#include <QVector>

class SimilarityEngine final
{
public:
    static constexpr qsizetype MaxTextLength = 200'000;
    static constexpr int MinNGramSize = 1;
    static constexpr int MaxNGramSize = 10;

    enum class NGramMode {
        Words,
        Characters
    };

    enum class SimilarityMetric {
        Jaccard,
        Cosine
    };

    struct Options {
        int n = 3;
        NGramMode mode = NGramMode::Words;
        SimilarityMetric metric = SimilarityMetric::Jaccard;
        bool caseInsensitive = true;
        bool ignorePunctuation = true;
        bool ignoreWhitespace = true;
    };

    struct SharedNGram {
        QString value;
        int countA = 0;
        int countB = 0;
        int sharedCount = 0;
    };

    struct Result {
        bool success = false;
        QString errorMessage;
        double similarity = 0.0;
        qsizetype totalNGramsA = 0;
        qsizetype totalNGramsB = 0;
        qsizetype uniqueNGramsA = 0;
        qsizetype uniqueNGramsB = 0;
        qsizetype sharedUniqueNGrams = 0;
        QVector<SharedNGram> commonNGrams;
    };

    static Result compare(const QString &textA,
                          const QString &textB,
                          const Options &options);


    static QString displayNGram(const QString &value, NGramMode mode);
};
