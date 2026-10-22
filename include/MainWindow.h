#pragma once

#include <QMainWindow>

#include "TextAnalyzer.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void analyzeTexts();
    void loadTextA();
    void loadTextB();
    void swapTexts();
    void exportReport();
    void invalidateResult();

private:
    static constexpr qsizetype MaxInputBytes = 256 * 1024;
    static constexpr int MaxVisibleCommonGrams = 1000;
    static constexpr int MaxReportCommonGrams = 10000;

    bool loadFileIntoEditor(QPlainTextEdit *editor, const QString &caption);
    TextSimilarity::Options currentOptions() const;
    void showResult(const TextSimilarity::AnalysisResult &result,
                    const TextSimilarity::Options &options);
    QString buildReport() const;
    void setStatus(const QString &message, bool isError = false);

    QPlainTextEdit *textA_ = nullptr;
    QPlainTextEdit *textB_ = nullptr;
    QComboBox *modeCombo_ = nullptr;
    QSpinBox *nSpinBox_ = nullptr;
    QCheckBox *ignoreCaseCheckBox_ = nullptr;
    QCheckBox *ignorePunctuationCheckBox_ = nullptr;
    QPushButton *analyzeButton_ = nullptr;
    QPushButton *exportButton_ = nullptr;
    QLabel *jaccardValueLabel_ = nullptr;
    QLabel *cosineValueLabel_ = nullptr;
    QLabel *statisticsLabel_ = nullptr;
    QLabel *commonSummaryLabel_ = nullptr;
    QTableWidget *commonTable_ = nullptr;
    QLabel *statusLabel_ = nullptr;

    bool hasResult_ = false;
    TextSimilarity::AnalysisResult lastResult_;
    TextSimilarity::Options lastOptions_;
};
