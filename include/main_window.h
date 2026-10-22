#pragma once // Защищает файл от повторного подключения

#include "similarity_engine.h" // Подключаем класс, который выполняет сравнение текстов

#include <QMainWindow> // Базовый класс главного окна Qt

    // Объявления классов Qt, которые используются через указатели
    class QCheckBox;      // Флажок для выбора параметров
class QComboBox;      // Выпадающий список
class QLabel;         // Текстовая надпись
class QProgressBar;   // Индикатор сходства
class QPushButton;    // Кнопка
class QSpinBox;       // Поле для ввода числа
class QTableWidget;   // Таблица с результатами
class QTextEdit;      // Поле для ввода текста

// Главное окно программы
class MainWindow final : public QMainWindow
{
    Q_OBJECT // Макрос Qt для сигналов и слотов

public:
    // Создание окна; parent позволяет указать родительский объект
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // Функции, которые вызываются при действиях пользователя
    void compareTexts(); // Сравнить два текста
    void loadTextA();    // Загрузить первый текст
    void loadTextB();    // Загрузить второй текст
    void clearTexts();   // Очистить оба поля
    void swapTexts();    // Поменять тексты местами
    void saveReport();   // Сохранить отчёт

private:
    // Вспомогательные функции для работы окна
    void buildUi(); // Создать интерфейс
    void loadTextFile(QTextEdit *target, const QString &caption); // Загрузить текст из файла
    void clearResultView(); // Очистить область результатов
    void showResult(const SimilarityEngine::Result &result,
                    const SimilarityEngine::Options &options); // Показать результаты анализа
    SimilarityEngine::Options currentOptions() const; // Получить выбранные настройки
    QString similarityInterpretation(double similarity) const; // Описать уровень сходства

    // Поля для ввода сравниваемых текстов
    QTextEdit *textA_ = nullptr;
    QTextEdit *textB_ = nullptr;

    // Элементы выбора параметров анализа
    QSpinBox *nSpinBox_ = nullptr;
    QComboBox *modeComboBox_ = nullptr;
    QComboBox *metricComboBox_ = nullptr;
    QCheckBox *caseInsensitiveCheckBox_ = nullptr;
    QCheckBox *ignorePunctuationCheckBox_ = nullptr;
    QCheckBox *ignoreWhitespaceCheckBox_ = nullptr;

    // Кнопки управления программой
    QPushButton *compareButton_ = nullptr;
    QPushButton *saveReportButton_ = nullptr;

    // Элементы для отображения результата
    QLabel *scoreLabel_ = nullptr;
    QLabel *interpretationLabel_ = nullptr;
    QLabel *statisticsLabel_ = nullptr;
    QProgressBar *similarityBar_ = nullptr;
    QTableWidget *commonNGramsTable_ = nullptr;

    // Храним последние результаты и настройки для сохранения отчёта
    SimilarityEngine::Result lastResult_;
    SimilarityEngine::Options lastOptions_;
    bool hasResult_ = false; // Было ли успешно выполнено сравнение
};
