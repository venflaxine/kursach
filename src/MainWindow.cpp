#include "MainWindow.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QStringList>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString modeName(TextSimilarity::GramMode mode)
{
    return mode == TextSimilarity::GramMode::Words
               ? QStringLiteral("по словам")
               : QStringLiteral("по символам");
}

QTableWidgetItem *makeReadOnlyItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Локальный антиплагиат — анализ сходства текстов"));
    resize(1180, 820);
    setMinimumSize(850, 650);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(12, 12, 12, 8);
    rootLayout->setSpacing(10);

    auto *titleLabel = new QLabel(
        QStringLiteral("Локальный антиплагиат\nАнализатор сходства текстов на N-граммах"), central);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 3);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setWordWrap(true);
    rootLayout->addWidget(titleLabel);

    auto *textsSplitter = new QSplitter(Qt::Horizontal, central);

    auto *groupA = new QGroupBox(QStringLiteral("Текст A"), textsSplitter);
    auto *layoutA = new QVBoxLayout(groupA);
    textA_ = new QPlainTextEdit(groupA);
    textA_->setPlaceholderText(QStringLiteral("Вставьте первый текст сюда или загрузите файл…"));
    textA_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto *buttonsA = new QHBoxLayout();
    auto *loadAButton = new QPushButton(QStringLiteral("Загрузить файл…"), groupA);
    buttonsA->addWidget(loadAButton);
    buttonsA->addStretch();
    layoutA->addWidget(textA_, 1);
    layoutA->addLayout(buttonsA);

    auto *groupB = new QGroupBox(QStringLiteral("Текст B"), textsSplitter);
    auto *layoutB = new QVBoxLayout(groupB);
    textB_ = new QPlainTextEdit(groupB);
    textB_->setPlaceholderText(QStringLiteral("Вставьте второй текст сюда или загрузите файл…"));
    textB_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto *buttonsB = new QHBoxLayout();
    auto *loadBButton = new QPushButton(QStringLiteral("Загрузить файл…"), groupB);
    buttonsB->addWidget(loadBButton);
    buttonsB->addStretch();
    layoutB->addWidget(textB_, 1);
    layoutB->addLayout(buttonsB);

    textsSplitter->addWidget(groupA);
    textsSplitter->addWidget(groupB);
    textsSplitter->setStretchFactor(0, 1);
    textsSplitter->setStretchFactor(1, 1);
    textsSplitter->setMinimumHeight(260);
    rootLayout->addWidget(textsSplitter, 3);

    auto *optionsGroup = new QGroupBox(QStringLiteral("Настройки анализа"), central);
    auto *optionsLayout = new QHBoxLayout(optionsGroup);
    auto *optionsForm = new QFormLayout();
    modeCombo_ = new QComboBox(optionsGroup);
    modeCombo_->addItem(QStringLiteral("N-граммы по словам"),
                        static_cast<int>(TextSimilarity::GramMode::Words));
    modeCombo_->addItem(QStringLiteral("N-граммы по символам"),
                        static_cast<int>(TextSimilarity::GramMode::Characters));
    nSpinBox_ = new QSpinBox(optionsGroup);
    nSpinBox_->setRange(1, 10);
    nSpinBox_->setValue(2);
    nSpinBox_->setToolTip(QStringLiteral("Количество соседних слов или символов в одной N-грамме"));
    optionsForm->addRow(QStringLiteral("Способ разбиения:"), modeCombo_);
    optionsForm->addRow(QStringLiteral("Размер N:"), nSpinBox_);
    optionsLayout->addLayout(optionsForm);

    auto *checksLayout = new QVBoxLayout();
    ignoreCaseCheckBox_ = new QCheckBox(QStringLiteral("Не учитывать регистр"), optionsGroup);
    ignoreCaseCheckBox_->setChecked(true);
    ignorePunctuationCheckBox_ = new QCheckBox(QStringLiteral("Игнорировать пунктуацию"), optionsGroup);
    ignorePunctuationCheckBox_->setChecked(true);
    checksLayout->addWidget(ignoreCaseCheckBox_);
    checksLayout->addWidget(ignorePunctuationCheckBox_);
    optionsLayout->addLayout(checksLayout, 1);

    auto *actionLayout = new QVBoxLayout();
    analyzeButton_ = new QPushButton(QStringLiteral("Сравнить тексты"), optionsGroup);
    analyzeButton_->setDefault(true);
    auto *swapButton = new QPushButton(QStringLiteral("Поменять тексты местами"), optionsGroup);
    exportButton_ = new QPushButton(QStringLiteral("Экспортировать отчёт…"), optionsGroup);
    exportButton_->setEnabled(false);
    actionLayout->addWidget(analyzeButton_);
    actionLayout->addWidget(swapButton);
    actionLayout->addWidget(exportButton_);
    optionsLayout->addLayout(actionLayout);
    rootLayout->addWidget(optionsGroup);

    auto *resultsGroup = new QGroupBox(QStringLiteral("Результаты"), central);
    auto *resultsLayout = new QVBoxLayout(resultsGroup);
    auto *metricsLayout = new QHBoxLayout();

    auto *jaccardBox = new QGroupBox(QStringLiteral("Коэффициент Жаккара"), resultsGroup);
    auto *jaccardLayout = new QVBoxLayout(jaccardBox);
    jaccardValueLabel_ = new QLabel(QStringLiteral("—"), jaccardBox);
    jaccardValueLabel_->setAlignment(Qt::AlignCenter);
    QFont metricFont = jaccardValueLabel_->font();
    metricFont.setPointSize(metricFont.pointSize() + 5);
    metricFont.setBold(true);
    jaccardValueLabel_->setFont(metricFont);
    jaccardLayout->addWidget(jaccardValueLabel_);

    auto *cosineBox = new QGroupBox(QStringLiteral("Косинусное сходство"), resultsGroup);
    auto *cosineLayout = new QVBoxLayout(cosineBox);
    cosineValueLabel_ = new QLabel(QStringLiteral("—"), cosineBox);
    cosineValueLabel_->setAlignment(Qt::AlignCenter);
    cosineValueLabel_->setFont(metricFont);
    cosineLayout->addWidget(cosineValueLabel_);

    metricsLayout->addWidget(jaccardBox);
    metricsLayout->addWidget(cosineBox);
    resultsLayout->addLayout(metricsLayout);

    statisticsLabel_ = new QLabel(
        QStringLiteral("Нажмите «Сравнить тексты», чтобы получить статистику."), resultsGroup);
    statisticsLabel_->setWordWrap(true);
    resultsLayout->addWidget(statisticsLabel_);

    commonSummaryLabel_ = new QLabel(QStringLiteral("Общие N-граммы появятся здесь."), resultsGroup);
    commonSummaryLabel_->setWordWrap(true);
    resultsLayout->addWidget(commonSummaryLabel_);

    commonTable_ = new QTableWidget(0, 3, resultsGroup);
    commonTable_->setHorizontalHeaderLabels(
        {QStringLiteral("Общая N-грамма"), QStringLiteral("Частота в A"), QStringLiteral("Частота в B")});
    commonTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    commonTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    commonTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    commonTable_->setAlternatingRowColors(true);
    commonTable_->verticalHeader()->setVisible(false);
    commonTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    commonTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    commonTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    commonTable_->setMinimumHeight(150);
    resultsLayout->addWidget(commonTable_, 1);
    rootLayout->addWidget(resultsGroup, 2);

    statusLabel_ = new QLabel(QStringLiteral("Готово к анализу. Обработка выполняется локально."), central);
    statusLabel_->setWordWrap(true);
    rootLayout->addWidget(statusLabel_);


    connect(loadAButton, &QPushButton::clicked, this, &MainWindow::loadTextA);
    connect(loadBButton, &QPushButton::clicked, this, &MainWindow::loadTextB);
    connect(analyzeButton_, &QPushButton::clicked, this, &MainWindow::analyzeTexts);
    connect(swapButton, &QPushButton::clicked, this, &MainWindow::swapTexts);
    connect(exportButton_, &QPushButton::clicked, this, &MainWindow::exportReport);

    connect(textA_, &QPlainTextEdit::textChanged, this, &MainWindow::invalidateResult);
    connect(textB_, &QPlainTextEdit::textChanged, this, &MainWindow::invalidateResult);
    connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::invalidateResult);
    connect(nSpinBox_, qOverload<int>(&QSpinBox::valueChanged),
            this, &MainWindow::invalidateResult);
    connect(ignoreCaseCheckBox_, &QCheckBox::toggled,
            this, &MainWindow::invalidateResult);
    connect(ignorePunctuationCheckBox_, &QCheckBox::toggled,
            this, &MainWindow::invalidateResult);
}

TextSimilarity::Options MainWindow::currentOptions() const
{
    TextSimilarity::Options options;
    options.n = nSpinBox_->value();
    options.mode = static_cast<TextSimilarity::GramMode>(modeCombo_->currentData().toInt());
    options.ignoreCase = ignoreCaseCheckBox_->isChecked();
    options.ignorePunctuation = ignorePunctuationCheckBox_->isChecked();
    return options;
}

void MainWindow::analyzeTexts()
{
    const QString textA = textA_->toPlainText();
    const QString textB = textB_->toPlainText();

    if (textA.toUtf8().size() > MaxInputBytes) {
        QMessageBox::warning(this, QStringLiteral("Слишком большой текст"),
            QStringLiteral("Текст A превышает ограничение в 256 КиБ UTF-8. Сократите текст и повторите попытку."));
        return;
    }
    if (textB.toUtf8().size() > MaxInputBytes) {
        QMessageBox::warning(this, QStringLiteral("Слишком большой текст"),
            QStringLiteral("Текст B превышает ограничение в 256 КиБ UTF-8. Сократите текст и повторите попытку."));
        return;
    }

    const auto options = currentOptions();
    const auto result = TextSimilarity::TextAnalyzer::compare(textA, textB, options);
    if (!result.ok) {
        hasResult_ = false;
        exportButton_->setEnabled(false);
        setStatus(result.error, true);
        QMessageBox::warning(this, QStringLiteral("Не удалось выполнить анализ"), result.error);
        return;
    }

    lastResult_ = result;
    lastOptions_ = options;
    hasResult_ = true;
    exportButton_->setEnabled(true);
    showResult(result, options);
    setStatus(QStringLiteral("Анализ завершён успешно."));
}

void MainWindow::showResult(const TextSimilarity::AnalysisResult &result,
                            const TextSimilarity::Options &options)
{
    jaccardValueLabel_->setText(QStringLiteral("%1 %").arg(result.jaccard * 100.0, 0, 'f', 2));
    cosineValueLabel_->setText(QStringLiteral("%1 %").arg(result.cosine * 100.0, 0, 'f', 2));

    statisticsLabel_->setText(
        QStringLiteral("Режим: %1, N = %2.\n"
                       "Текст A: %3 N-грамм вхождений, %4 уникальных.\n"
                       "Текст B: %5 N-грамм вхождений, %6 уникальных.\n"
                       "Совпадающих уникальных N-грамм: %7.")
            .arg(modeName(options.mode))
            .arg(options.n)
            .arg(result.totalNgramsA)
            .arg(result.uniqueNgramsA)
            .arg(result.totalNgramsB)
            .arg(result.uniqueNgramsB)
            .arg(result.commonUniqueNgrams));

    const int rowsToShow = qMin<int>(MaxVisibleCommonGrams, static_cast<int>(result.commonGrams.size()));
    commonTable_->setRowCount(rowsToShow);
    for (int row = 0; row < rowsToShow; ++row) {
        const auto &gram = result.commonGrams.at(row);
        commonTable_->setItem(row, 0,
            makeReadOnlyItem(TextSimilarity::TextAnalyzer::gramToDisplay(gram.gram, options.mode)));
        commonTable_->setItem(row, 1, makeReadOnlyItem(QString::number(gram.countA)));
        commonTable_->setItem(row, 2, makeReadOnlyItem(QString::number(gram.countB)));
    }

    if (result.commonGrams.isEmpty()) {
        commonSummaryLabel_->setText(QStringLiteral(
            "Общих N-грамм не найдено. Это не означает автоматически, что тексты не связаны."));
    } else if (result.commonGrams.size() > rowsToShow) {
        commonSummaryLabel_->setText(
            QStringLiteral("Найдено общих уникальных N-грамм: %1. В таблице показаны первые %2.")
                .arg(result.commonGrams.size()).arg(rowsToShow));
    } else {
        commonSummaryLabel_->setText(
            QStringLiteral("Найдено общих уникальных N-грамм: %1.").arg(result.commonGrams.size()));
    }
}

bool MainWindow::loadFileIntoEditor(QPlainTextEdit *editor, const QString &caption)
{
    const QString path = QFileDialog::getOpenFileName(
        this, caption, QString(),
        QStringLiteral("Текстовые файлы (*.txt *.md);;Все файлы (*.*)"));
    if (path.isEmpty())
        return false;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("Ошибка открытия файла"),
            QStringLiteral("Не удалось открыть файл:\n%1\n\nПричина: %2")
                .arg(path, file.errorString()));
        return false;
    }

    if (file.size() > MaxInputBytes) {
        QMessageBox::warning(this, QStringLiteral("Файл слишком большой"),
            QStringLiteral("Размер файла превышает ограничение в 256 КиБ."));
        return false;
    }

    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(this, QStringLiteral("Ошибка чтения файла"), file.errorString());
        return false;
    }

    const QString decoded = QString::fromUtf8(bytes.constData(), bytes.size());
    if (decoded.toUtf8() != bytes) {
        QMessageBox::warning(this, QStringLiteral("Некорректная кодировка"),
            QStringLiteral("Файл не является корректным UTF-8. Пересохраните его в кодировке UTF-8 и попробуйте снова."));
        return false;
    }

    editor->setPlainText(decoded);
    setStatus(QStringLiteral("Файл загружен: %1").arg(path));
    return true;
}

void MainWindow::loadTextA()
{
    loadFileIntoEditor(textA_, QStringLiteral("Загрузить текст A"));
}

void MainWindow::loadTextB()
{
    loadFileIntoEditor(textB_, QStringLiteral("Загрузить текст B"));
}

void MainWindow::swapTexts()
{
    const QString temporary = textA_->toPlainText();
    textA_->setPlainText(textB_->toPlainText());
    textB_->setPlainText(temporary);
    setStatus(QStringLiteral("Тексты поменяны местами. Запустите анализ повторно."));
}

QString MainWindow::buildReport() const
{
    QStringList lines;
    lines << QStringLiteral("ОТЧЁТ: ЛОКАЛЬНЫЙ АНТИПЛАГИАТ")
          << QStringLiteral("Сформирован: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate))
          << QStringLiteral("Обработка выполнена локально; исходные тексты не включены в отчёт.")
          << QString()
          << QStringLiteral("НАСТРОЙКИ")
          << QStringLiteral("Режим: %1").arg(modeName(lastOptions_.mode))
          << QStringLiteral("Размер N: %1").arg(lastOptions_.n)
          << QStringLiteral("Игнорировать регистр: %1").arg(lastOptions_.ignoreCase ? QStringLiteral("да") : QStringLiteral("нет"))
          << QStringLiteral("Игнорировать пунктуацию: %1").arg(lastOptions_.ignorePunctuation ? QStringLiteral("да") : QStringLiteral("нет"))
          << QString()
          << QStringLiteral("РЕЗУЛЬТАТЫ")
          << QStringLiteral("Жаккар: %1 %").arg(lastResult_.jaccard * 100.0, 0, 'f', 2)
          << QStringLiteral("Косинусное сходство: %1 %").arg(lastResult_.cosine * 100.0, 0, 'f', 2)
          << QStringLiteral("N-грамм вхождений в A: %1").arg(lastResult_.totalNgramsA)
          << QStringLiteral("N-грамм вхождений в B: %1").arg(lastResult_.totalNgramsB)
          << QStringLiteral("Уникальных N-грамм в A: %1").arg(lastResult_.uniqueNgramsA)
          << QStringLiteral("Уникальных N-грамм в B: %1").arg(lastResult_.uniqueNgramsB)
          << QStringLiteral("Общих уникальных N-грамм: %1").arg(lastResult_.commonUniqueNgrams)
          << QString()
          << QStringLiteral("ОБЩИЕ N-ГРАММЫ")
          << QStringLiteral("N-грамма\tЧастота A\tЧастота B");

    const int rows = qMin<int>(MaxReportCommonGrams, static_cast<int>(lastResult_.commonGrams.size()));
    for (int i = 0; i < rows; ++i) {
        const auto &gram = lastResult_.commonGrams.at(i);
        lines << QStringLiteral("%1\t%2\t%3")
                     .arg(TextSimilarity::TextAnalyzer::gramToDisplay(gram.gram, lastOptions_.mode))
                     .arg(gram.countA)
                     .arg(gram.countB);
    }
    if (lastResult_.commonGrams.size() > rows) {
        lines << QString()
              << QStringLiteral("Примечание: в отчёт включены первые %1 из %2 общих уникальных N-грамм.")
                     .arg(rows).arg(lastResult_.commonGrams.size());
    }
    return lines.join(QLatin1Char('\n'));
}

void MainWindow::exportReport()
{
    if (!hasResult_) {
        QMessageBox::information(this, QStringLiteral("Нет результата"),
            QStringLiteral("Сначала выполните сравнение двух текстов."));
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить отчёт"), QStringLiteral("similarity_report.txt"),
        QStringLiteral("Текстовые отчёты (*.txt)"));
    if (path.isEmpty())
        return;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, QStringLiteral("Ошибка сохранения"), file.errorString());
        return;
    }

    const QByteArray bytes = buildReport().toUtf8();
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        QMessageBox::critical(this, QStringLiteral("Ошибка записи"),
            QStringLiteral("Не удалось полностью записать отчёт: %1").arg(file.errorString()));
        return;
    }
    if (!file.commit()) {
        QMessageBox::critical(this, QStringLiteral("Ошибка сохранения"), file.errorString());
        return;
    }

    setStatus(QStringLiteral("Отчёт сохранён: %1").arg(path));
    QMessageBox::information(this, QStringLiteral("Отчёт сохранён"),
        QStringLiteral("Отчёт успешно сохранён в файл:\n%1").arg(path));
}

void MainWindow::invalidateResult()
{
    if (!hasResult_ && commonTable_->rowCount() == 0)
        return;

    hasResult_ = false;
    exportButton_->setEnabled(false);
    jaccardValueLabel_->setText(QStringLiteral("—"));
    cosineValueLabel_->setText(QStringLiteral("—"));
    statisticsLabel_->setText(QStringLiteral("Настройки или тексты изменены. Повторите анализ."));
    commonSummaryLabel_->setText(QStringLiteral("Результаты устарели; запустите анализ заново."));
    commonTable_->setRowCount(0);
}

void MainWindow::setStatus(const QString &message, bool isError)
{
    statusLabel_->setText(message);
    statusLabel_->setStyleSheet(isError
        ? QStringLiteral("QLabel { color: #b91c1c; }")
        : QString());
    statusBar()->showMessage(message, 7000);
}
