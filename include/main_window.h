#pragma once

#include "similarity_engine.h"

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTextEdit;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void compareTexts();
    void loadTextA();
    void loadTextB();
    void clearTexts();
    void swapTexts();
    void saveReport();

private:
    void buildUi();
    void loadTextFile(QTextEdit *target, const QString &caption);
    void clearResultView();
    void showResult(const SimilarityEngine::Result &result,
                    const SimilarityEngine::Options &options);
    SimilarityEngine::Options currentOptions() const;
    QString similarityInterpretation(double similarity) const;

    QTextEdit *textA_ = nullptr;
    QTextEdit *textB_ = nullptr;
    QSpinBox *nSpinBox_ = nullptr;
    QComboBox *modeComboBox_ = nullptr;
    QComboBox *metricComboBox_ = nullptr;
    QCheckBox *caseInsensitiveCheckBox_ = nullptr;
    QCheckBox *ignorePunctuationCheckBox_ = nullptr;
    QCheckBox *ignoreWhitespaceCheckBox_ = nullptr;
    QPushButton *compareButton_ = nullptr;
    QPushButton *saveReportButton_ = nullptr;
    QLabel *scoreLabel_ = nullptr;
    QLabel *interpretationLabel_ = nullptr;
    QLabel *statisticsLabel_ = nullptr;
    QProgressBar *similarityBar_ = nullptr;
    QTableWidget *commonNGramsTable_ = nullptr;

    SimilarityEngine::Result lastResult_;
    SimilarityEngine::Options lastOptions_;
    bool hasResult_ = false;
};
