#pragma once

#include <QMap>
#include <QString>
#include <QVector>

namespace TextSimilarity {

enum class GramMode {
    Words,
    Characters
};

struct Options {
    int n = 2;
    GramMode mode = GramMode::Words;
    bool ignoreCase = true;
    bool ignorePunctuation = true;
};

struct CommonGram {
    QString gram;
    int countA = 0;
    int countB = 0;
};

struct AnalysisResult {
    bool ok = false;
    QString error;

    double jaccard = 0.0;
    double cosine = 0.0;

    int totalNgramsA = 0;
    int totalNgramsB = 0;
    int uniqueNgramsA = 0;
    int uniqueNgramsB = 0;
    int commonUniqueNgrams = 0;

    QVector<CommonGram> commonGrams;
};

class TextAnalyzer final {
public:
    static AnalysisResult compare(const QString &textA,
                                  const QString &textB,
                                  const Options &options);

    static QString gramToDisplay(const QString &encodedGram, GramMode mode);

private:
    using FrequencyMap = QMap<QString, int>;

    static QVector<QString> tokenize(const QString &text, const Options &options);
    static FrequencyMap buildNgrams(const QVector<QString> &tokens,
                                    const Options &options);
    static int sumFrequencies(const FrequencyMap &frequencies);
    static bool isUnicodePunctuation(char32_t codePoint);
};

}
