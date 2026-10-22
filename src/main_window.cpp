#include "main_window.h" // Заголовочный файл главного окна программы

// Элементы интерфейса и их настройка
#include <QAbstractItemView>  // Настройка поведения таблицы
#include <QCheckBox>          // Флажки для выбора параметров
#include <QComboBox>          // Выпадающие списки
#include <QGroupBox>          // Группировка элементов в рамки
#include <QHeaderView>        // Настройка заголовков таблицы
#include <QLabel>             // Текстовые надписи
#include <QProgressBar>       // Полоса прогресса для отображения сходства
#include <QPushButton>        // Кнопки
#include <QScrollArea>        // Область с прокруткой
#include <QSpinBox>           // Поле для ввода целого числа
#include <QSplitter>          // Разделитель между областями интерфейса
#include <QStatusBar>          // Строка состояния внизу окна
#include <QTableWidget>       // Таблица результатов
#include <QTableWidgetItem>   // Отдельные ячейки таблицы
#include <QTextEdit>          // Поля для ввода текстов
#include <QWidget>            // Базовый класс элементов интерфейса

// Компоновка элементов интерфейса
#include <QHBoxLayout>        // Расположение элементов по горизонтали
#include <QVBoxLayout>        // Расположение элементов по вертикали

// Работа с файлами и диалоговыми окнами
#include <QFile>              // Чтение и запись файлов
#include <QFileDialog>        // Окна выбора файла и сохранения отчёта
#include <QFileInfo>          // Получение информации о файле, например его размера
#include <QTextStream>        // Запись текста в файл
#include <QStringConverter>   // Настройка кодировки при записи файла

// Сообщения, дата и время
#include <QMessageBox>        // Предупреждения и сообщения пользователю
#include <QDateTime>           // Получение текущей даты и времени
#include <algorithm>

    namespace {
    constexpr qint64 MaxTextFileBytes = 2 * 1024 * 1024;
    constexpr int MaxDisplayedCommonNGrams = 100;

    // Заголовок для отдельных блоков интерфейса.
    QLabel *makeSectionTitle(const QString &text)
    {
        auto *label = new QLabel(text);
        label->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
        return label;
    }

    // Названия режимов и метрик для вывода в интерфейсе.
    QString nGramModeName(SimilarityEngine::NGramMode mode)
    {
        return mode == SimilarityEngine::NGramMode::Words
                   ? QStringLiteral("словесные N-граммы")
                   : QStringLiteral("символьные N-граммы");
    }

    QString metricName(SimilarityEngine::SimilarityMetric metric)
    {
        return metric == SimilarityEngine::SimilarityMetric::Jaccard
                   ? QStringLiteral("коэффициент Жаккара")
                   : QStringLiteral("косинусное сходство");
    }
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    setWindowTitle(QStringLiteral("Локальный антиплагиат — анализатор сходства N-грамм"));
    resize(1180, 850);
    setMinimumSize(900, 650);
}

// Создание интерфейса и подключение кнопок.
void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(18, 16, 18, 14);
    rootLayout->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("Локальный антиплагиат"));
    title->setStyleSheet(QStringLiteral("font-size: 26px; font-weight: 700; color: #1d4ed8;"));
    auto *subtitle = new QLabel(QStringLiteral(
        "Сравнение двух текстов по N-граммам."));
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QStringLiteral("color: #475569;"));
    rootLayout->addWidget(title);
    rootLayout->addWidget(subtitle);

    // Поля для двух сравниваемых текстов.
    auto *textsSplitter = new QSplitter(Qt::Horizontal, central);
    auto *groupA = new QGroupBox(QStringLiteral("Текст A"), textsSplitter);
    auto *layoutA = new QVBoxLayout(groupA);
    auto *loadAButton = new QPushButton(QStringLiteral("Загрузить .txt…"), groupA);
    textA_ = new QTextEdit(groupA);
    textA_->setAcceptRichText(false);
    textA_->setTabChangesFocus(true);
    textA_->setPlaceholderText(QStringLiteral("Вставьте сюда первый текст или загрузите UTF-8 файл…"));
    textA_->setMinimumHeight(190);
    layoutA->addWidget(loadAButton, 0, Qt::AlignLeft);
    layoutA->addWidget(textA_, 1);

    auto *groupB = new QGroupBox(QStringLiteral("Текст B"), textsSplitter);
    auto *layoutB = new QVBoxLayout(groupB);
    auto *loadBButton = new QPushButton(QStringLiteral("Загрузить .txt…"), groupB);
    textB_ = new QTextEdit(groupB);
    textB_->setAcceptRichText(false);
    textB_->setTabChangesFocus(true);
    textB_->setPlaceholderText(QStringLiteral("Вставьте сюда второй текст или загрузите UTF-8 файл…"));
    textB_->setMinimumHeight(190);
    layoutB->addWidget(loadBButton, 0, Qt::AlignLeft);
    layoutB->addWidget(textB_, 1);

    textsSplitter->addWidget(groupA);
    textsSplitter->addWidget(groupB);
    textsSplitter->setStretchFactor(0, 1);
    textsSplitter->setStretchFactor(1, 1);
    rootLayout->addWidget(textsSplitter, 3);

    // Настройки сравнения.
    auto *settingsGroup = new QGroupBox(QStringLiteral("Параметры анализа"), central);
    auto *settingsLayout = new QVBoxLayout(settingsGroup);
    auto *settingsRow = new QHBoxLayout();

    settingsRow->addWidget(new QLabel(QStringLiteral("Размер N:"), settingsGroup));
    nSpinBox_ = new QSpinBox(settingsGroup);
    nSpinBox_->setRange(SimilarityEngine::MinNGramSize, SimilarityEngine::MaxNGramSize);
    nSpinBox_->setValue(3);
    nSpinBox_->setToolTip(QStringLiteral("Сколько последовательных слов или символов образует одну N-грамму."));
    settingsRow->addWidget(nSpinBox_);

    settingsRow->addSpacing(12);
    settingsRow->addWidget(new QLabel(QStringLiteral("Тип:"), settingsGroup));
    modeComboBox_ = new QComboBox(settingsGroup);
    modeComboBox_->addItem(QStringLiteral("По словам"), static_cast<int>(SimilarityEngine::NGramMode::Words));
    modeComboBox_->addItem(QStringLiteral("По символам"), static_cast<int>(SimilarityEngine::NGramMode::Characters));
    modeComboBox_->setToolTip(QStringLiteral("Словесные N-граммы сравнивают соседние слова, символьные — соседние символы."));
    settingsRow->addWidget(modeComboBox_);

    settingsRow->addSpacing(12);
    settingsRow->addWidget(new QLabel(QStringLiteral("Метрика:"), settingsGroup));
    metricComboBox_ = new QComboBox(settingsGroup);
    metricComboBox_->addItem(QStringLiteral("Жаккар (множества)"), static_cast<int>(SimilarityEngine::SimilarityMetric::Jaccard));
    metricComboBox_->addItem(QStringLiteral("Косинус (частоты)"), static_cast<int>(SimilarityEngine::SimilarityMetric::Cosine));
    metricComboBox_->setToolTip(QStringLiteral("Жаккар учитывает наличие уникальных N-грамм, косинус — ещё и частоты повторения."));
    settingsRow->addWidget(metricComboBox_);
    settingsRow->addStretch(1);
    settingsLayout->addLayout(settingsRow);

    auto *flagsRow = new QHBoxLayout();
    caseInsensitiveCheckBox_ = new QCheckBox(QStringLiteral("Не учитывать регистр"), settingsGroup);
    caseInsensitiveCheckBox_->setChecked(true);
    ignorePunctuationCheckBox_ = new QCheckBox(QStringLiteral("Игнорировать пунктуацию"), settingsGroup);
    ignorePunctuationCheckBox_->setChecked(true);
    ignorePunctuationCheckBox_->setToolTip(QStringLiteral("Применяется в режиме символьных N-грамм; в режиме слов знаки препинания служат разделителями."));
    ignoreWhitespaceCheckBox_ = new QCheckBox(QStringLiteral("Игнорировать пробельные символы"), settingsGroup);
    ignoreWhitespaceCheckBox_->setChecked(true);
    ignoreWhitespaceCheckBox_->setToolTip(QStringLiteral("В символьном режиме удаляет пробелы, табуляции и переносы строк перед построением N-грамм."));
    flagsRow->addWidget(caseInsensitiveCheckBox_);
    flagsRow->addWidget(ignorePunctuationCheckBox_);
    flagsRow->addWidget(ignoreWhitespaceCheckBox_);
    flagsRow->addStretch(1);
    settingsLayout->addLayout(flagsRow);
    rootLayout->addWidget(settingsGroup, 0);

    auto *buttonsRow = new QHBoxLayout();
    compareButton_ = new QPushButton(QStringLiteral("Сравнить тексты"), central);
    compareButton_->setDefault(true);
    compareButton_->setStyleSheet(QStringLiteral(
        "QPushButton { background: #2563eb; color: white; font-weight: 600; padding: 9px 20px; border-radius: 5px; }"
        "QPushButton:hover { background: #1d4ed8; }"
        "QPushButton:disabled { background: #94a3b8; }"));
    auto *swapButton = new QPushButton(QStringLiteral("Поменять местами"), central);
    auto *clearButton = new QPushButton(QStringLiteral("Очистить"), central);
    saveReportButton_ = new QPushButton(QStringLiteral("Сохранить отчёт…"), central);
    saveReportButton_->setEnabled(false);
    buttonsRow->addWidget(compareButton_);
    buttonsRow->addWidget(swapButton);
    buttonsRow->addWidget(clearButton);
    buttonsRow->addStretch(1);
    buttonsRow->addWidget(saveReportButton_);
    rootLayout->addLayout(buttonsRow);

    // Область с результатами анализа.
    auto *resultsGroup = new QGroupBox(QStringLiteral("Результаты"), central);
    auto *resultsLayout = new QVBoxLayout(resultsGroup);
    auto *scoreRow = new QHBoxLayout();
    scoreLabel_ = new QLabel(QStringLiteral("— %"), resultsGroup);
    scoreLabel_->setMinimumWidth(120);
    scoreLabel_->setStyleSheet(QStringLiteral("font-size: 28px; font-weight: 700; color: #0f172a;"));
    auto *scoreDetails = new QVBoxLayout();
    interpretationLabel_ = new QLabel(QStringLiteral("Вставьте два текста и нажмите «Сравнить тексты»."), resultsGroup);
    interpretationLabel_->setWordWrap(true);
    interpretationLabel_->setStyleSheet(QStringLiteral("font-weight: 600;"));
    similarityBar_ = new QProgressBar(resultsGroup);
    similarityBar_->setRange(0, 1000);
    similarityBar_->setValue(0);
    similarityBar_->setFormat(QStringLiteral("Условный индикатор сходства: %p%"));
    scoreDetails->addWidget(interpretationLabel_);
    scoreDetails->addWidget(similarityBar_);
    scoreRow->addWidget(scoreLabel_);
    scoreRow->addLayout(scoreDetails, 1);
    resultsLayout->addLayout(scoreRow);

    statisticsLabel_ = new QLabel(QStringLiteral("Статистика появится после анализа."), resultsGroup);
    statisticsLabel_->setWordWrap(true);
    statisticsLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    resultsLayout->addWidget(statisticsLabel_);

    resultsLayout->addWidget(makeSectionTitle(QStringLiteral("Общие N-граммы (до 100 наиболее частых)")));
    commonNGramsTable_ = new QTableWidget(0, 4, resultsGroup);
    commonNGramsTable_->setHorizontalHeaderLabels({
        QStringLiteral("N-грамма"), QStringLiteral("В тексте A"),
        QStringLiteral("В тексте B"), QStringLiteral("Общих вхождений")
    });
    commonNGramsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    commonNGramsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    commonNGramsTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    commonNGramsTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    commonNGramsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    commonNGramsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    commonNGramsTable_->setAlternatingRowColors(true);
    commonNGramsTable_->setMinimumHeight(150);
    resultsLayout->addWidget(commonNGramsTable_, 1);
    rootLayout->addWidget(resultsGroup, 2);

    setCentralWidget(central);

    // Связываем кнопки с обработчиками.
    connect(loadAButton, &QPushButton::clicked, this, &MainWindow::loadTextA);
    connect(loadBButton, &QPushButton::clicked, this, &MainWindow::loadTextB);
    connect(compareButton_, &QPushButton::clicked, this, &MainWindow::compareTexts);
    connect(swapButton, &QPushButton::clicked, this, &MainWindow::swapTexts);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearTexts);
    connect(saveReportButton_, &QPushButton::clicked, this, &MainWindow::saveReport);
}

// Собирает текущие значения настроек из интерфейса.
SimilarityEngine::Options MainWindow::currentOptions() const
{
    SimilarityEngine::Options options;
    options.n = nSpinBox_->value();
    options.mode = static_cast<SimilarityEngine::NGramMode>(modeComboBox_->currentData().toInt());
    options.metric = static_cast<SimilarityEngine::SimilarityMetric>(metricComboBox_->currentData().toInt());
    options.caseInsensitive = caseInsensitiveCheckBox_->isChecked();
    options.ignorePunctuation = ignorePunctuationCheckBox_->isChecked();
    options.ignoreWhitespace = ignoreWhitespaceCheckBox_->isChecked();
    return options;
}

void MainWindow::compareTexts()
{
    compareButton_->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("Сравнение…"));

    const auto options = currentOptions();
    const auto result = SimilarityEngine::compare(textA_->toPlainText(), textB_->toPlainText(), options);

    if (!result.success) {
        hasResult_ = false;
        saveReportButton_->setEnabled(false);
        clearResultView();
        statusBar()->showMessage(result.errorMessage, 10000);
        QMessageBox::warning(this, QStringLiteral("Не удалось сравнить тексты"), result.errorMessage);
        compareButton_->setEnabled(true);
        return;
    }

    // Сохраняем последний успешный результат для отчёта.
    lastResult_ = result;
    lastOptions_ = options;
    hasResult_ = true;
    saveReportButton_->setEnabled(true);
    showResult(result, options);
    statusBar()->showMessage(QStringLiteral("Анализ завершён."), 5000);
    compareButton_->setEnabled(true);
}

void MainWindow::loadTextA()
{
    loadTextFile(textA_, QStringLiteral("Выберите файл для текста A"));
}

void MainWindow::loadTextB()
{
    loadTextFile(textB_, QStringLiteral("Выберите файл для текста B"));
}

// Загрузка файла с проверкой размера, чтения и кодировки.
void MainWindow::loadTextFile(QTextEdit *target, const QString &caption)
{
    const QString path = QFileDialog::getOpenFileName(
        this, caption, QString(), QStringLiteral("Текстовые файлы (*.txt *.md);;Все файлы (*.*)"));
    if (path.isEmpty()) {
        return;
    }

    const QFileInfo fileInfo(path);
    if (fileInfo.size() > MaxTextFileBytes) {
        QMessageBox::warning(this, QStringLiteral("Файл слишком большой"),
                             QStringLiteral("Максимальный размер файла — 2 MiB. Выбранный файл: %1 MiB.")
                                 .arg(static_cast<double>(fileInfo.size()) / (1024.0 * 1024.0), 0, 'f', 2));
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("Ошибка чтения"),
                              QStringLiteral("Не удалось открыть файл:\n%1\n\n%2")
                                  .arg(path, file.errorString()));
        return;
    }

    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(this, QStringLiteral("Ошибка чтения"),
                              QStringLiteral("Не удалось полностью прочитать файл:\n%1")
                                  .arg(file.errorString()));
        return;
    }

    QString decoded = QString::fromUtf8(bytes.constData(), bytes.size());
    if (decoded.toUtf8() != bytes) {
        QMessageBox::warning(this, QStringLiteral("Неверная кодировка"),
                             QStringLiteral("Файл не выглядит как корректный UTF-8. Сохраните его в UTF-8 и повторите загрузку."));
        return;
    }
    if (!decoded.isEmpty() && decoded.front() == QChar(static_cast<char16_t>(0xFEFF))) {
        decoded.remove(0, 1); // Убираем BOM в начале файла, если он есть.
    }
    if (decoded.size() > SimilarityEngine::MaxTextLength) {
        QMessageBox::warning(this, QStringLiteral("Текст слишком большой"),
                             QStringLiteral("Максимальная длина текста — %1 символов.")
                                 .arg(SimilarityEngine::MaxTextLength));
        return;
    }

    target->setPlainText(decoded);
    hasResult_ = false;
    saveReportButton_->setEnabled(false);
    clearResultView();
    statusBar()->showMessage(QStringLiteral("Загружен файл: %1").arg(fileInfo.fileName()), 5000);
}

void MainWindow::clearTexts()
{
    textA_->clear();
    textB_->clear();
    hasResult_ = false;
    saveReportButton_->setEnabled(false);
    clearResultView();
    statusBar()->showMessage(QStringLiteral("Поля очищены."), 3000);
}

void MainWindow::swapTexts()
{
    const QString oldA = textA_->toPlainText();
    textA_->setPlainText(textB_->toPlainText());
    textB_->setPlainText(oldA);
    hasResult_ = false;
    saveReportButton_->setEnabled(false);
    clearResultView();
    statusBar()->showMessage(QStringLiteral("Тексты поменяны местами."), 3000);
}

// Возвращает область результатов в исходное состояние.
void MainWindow::clearResultView()
{
    scoreLabel_->setText(QStringLiteral("— %"));
    interpretationLabel_->setText(QStringLiteral("Вставьте два текста и нажмите «Сравнить тексты»."));
    similarityBar_->setValue(0);
    statisticsLabel_->setText(QStringLiteral("Статистика появится после анализа."));
    commonNGramsTable_->setRowCount(0);
}

QString MainWindow::similarityInterpretation(double similarity) const
{
    if (similarity < 0.20) {
        return QStringLiteral("Низкое лексическое сходство (условная шкала)");
    }
    if (similarity < 0.50) {
        return QStringLiteral("Умеренное лексическое сходство (условная шкала)");
    }
    if (similarity < 0.75) {
        return QStringLiteral("Высокое лексическое сходство (условная шкала)");
    }
    return QStringLiteral("Очень высокое лексическое сходство (условная шкала)");
}

void MainWindow::showResult(const SimilarityEngine::Result &result,
                            const SimilarityEngine::Options &options)
{
    const double percent = result.similarity * 100.0;
    scoreLabel_->setText(QStringLiteral("%1%").arg(percent, 0, 'f', 2));
    interpretationLabel_->setText(similarityInterpretation(result.similarity));
    similarityBar_->setValue(qRound(result.similarity * 1000.0));
    statisticsLabel_->setText(QStringLiteral(
                                  "Текст A: N-грамм — %1, уникальных — %2.  "
                                  "Текст B: N-грамм — %3, уникальных — %4.\n"
                                  "Общих уникальных N-грамм — %5.  Метод: %6; тип: %7; N = %8.")
                                  .arg(result.totalNGramsA)
                                  .arg(result.uniqueNGramsA)
                                  .arg(result.totalNGramsB)
                                  .arg(result.uniqueNGramsB)
                                  .arg(result.sharedUniqueNGrams)
                                  .arg(metricName(options.metric), nGramModeName(options.mode))
                                  .arg(options.n));

    // Заполняем таблицу общих N-грамм.
    const int rows = static_cast<int>(std::min<qsizetype>(result.commonNGrams.size(), MaxDisplayedCommonNGrams));
    commonNGramsTable_->setRowCount(rows);
    for (int row = 0; row < rows; ++row) {
        const auto &ngram = result.commonNGrams.at(row);
        auto *gramItem = new QTableWidgetItem(SimilarityEngine::displayNGram(ngram.value, options.mode));
        gramItem->setToolTip(gramItem->text());
        commonNGramsTable_->setItem(row, 0, gramItem);
        commonNGramsTable_->setItem(row, 1, new QTableWidgetItem(QString::number(ngram.countA)));
        commonNGramsTable_->setItem(row, 2, new QTableWidgetItem(QString::number(ngram.countB)));
        commonNGramsTable_->setItem(row, 3, new QTableWidgetItem(QString::number(ngram.sharedCount)));
    }

    if (result.commonNGrams.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Общих N-грамм не найдено."), 5000);
    } else if (result.commonNGrams.size() > MaxDisplayedCommonNGrams) {
        statusBar()->showMessage(QStringLiteral("В таблице показаны первые %1 из %2 общих N-грамм.")
                                     .arg(MaxDisplayedCommonNGrams)
                                     .arg(result.commonNGrams.size()), 6000);
    }
}

// Сохраняет результат последнего сравнения в текстовый файл.
void MainWindow::saveReport()
{
    if (!hasResult_) {
        QMessageBox::information(this, QStringLiteral("Нет результата"),
                                 QStringLiteral("Сначала успешно сравните два текста."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить отчёт"), QStringLiteral("similarity_report.txt"),
        QStringLiteral("Текстовый отчёт (*.txt)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".txt");
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("Ошибка сохранения"),
                              QStringLiteral("Не удалось сохранить отчёт:\n%1\n\n%2")
                                  .arg(path, file.errorString()));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << QStringLiteral("ЛОКАЛЬНЫЙ АНТИПЛАГИАТ — ОТЧЁТ\n");
    out << QStringLiteral("Дата анализа: ") << QDateTime::currentDateTime().toString(Qt::ISODate) << QLatin1Char('\n');
    out << QStringLiteral("Сходство: ") << QString::number(lastResult_.similarity * 100.0, 'f', 2) << QStringLiteral("%\n");
    out << QStringLiteral("Интерпретация: ") << similarityInterpretation(lastResult_.similarity) << QLatin1Char('\n');
    out << QStringLiteral("Тип N-грамм: ") << nGramModeName(lastOptions_.mode) << QLatin1Char('\n');
    out << QStringLiteral("Метрика: ") << metricName(lastOptions_.metric) << QLatin1Char('\n');
    out << QStringLiteral("N: ") << lastOptions_.n << QLatin1Char('\n');
    out << QStringLiteral("Без учёта регистра: ")
        << (lastOptions_.caseInsensitive ? QStringLiteral("да") : QStringLiteral("нет")) << QLatin1Char('\n');
    out << QStringLiteral("Игнорировать пунктуацию: ")
        << (lastOptions_.ignorePunctuation ? QStringLiteral("да") : QStringLiteral("нет")) << QLatin1Char('\n');
    out << QStringLiteral("Игнорировать пробелы: ")
        << (lastOptions_.ignoreWhitespace ? QStringLiteral("да") : QStringLiteral("нет")) << QStringLiteral("\n\n");
    out << QStringLiteral("Текст A — всего N-грамм: ") << lastResult_.totalNGramsA
        << QStringLiteral(", уникальных: ") << lastResult_.uniqueNGramsA << QLatin1Char('\n');
    out << QStringLiteral("Текст B — всего N-грамм: ") << lastResult_.totalNGramsB
        << QStringLiteral(", уникальных: ") << lastResult_.uniqueNGramsB << QLatin1Char('\n');
    out << QStringLiteral("Общих уникальных N-грамм: ") << lastResult_.sharedUniqueNGrams << QStringLiteral("\n\n");
    out << QStringLiteral("ОБЩИЕ N-ГРАММЫ (первые 100 по частоте совпадений)\n");
    out << QStringLiteral("N-грамма\tЧастота A\tЧастота B\tОбщих вхождений\n");

    const int limit = static_cast<int>(std::min<qsizetype>(lastResult_.commonNGrams.size(), MaxDisplayedCommonNGrams));
    for (int i = 0; i < limit; ++i) {
        const auto &ngram = lastResult_.commonNGrams.at(i);
        out << SimilarityEngine::displayNGram(ngram.value, lastOptions_.mode) << QLatin1Char('\t')
            << ngram.countA << QLatin1Char('\t') << ngram.countB << QLatin1Char('\t')
            << ngram.sharedCount << QLatin1Char('\n');
    }

    out.flush();
    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(this, QStringLiteral("Ошибка записи"),
                              QStringLiteral("Во время записи отчёта произошла ошибка:\n%1")
                                  .arg(file.errorString()));
        return;
    }

    statusBar()->showMessage(QStringLiteral("Отчёт сохранён: %1").arg(QFileInfo(path).fileName()), 5000);
    QMessageBox::information(this, QStringLiteral("Отчёт сохранён"),
                             QStringLiteral("Отчёт успешно сохранён:\n%1").arg(path));
}