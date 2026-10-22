#include "main_window.h" // Подключаем заголовочный файл главного окна программы

// Библиотеки Qt
#include <QApplication>      // Управление приложением с графическим интерфейсом
#include <QCoreApplication>  // Настройка общих параметров приложения

int main(int argc, char *argv[])
{
    // Создаём приложение Qt и передаём ему аргументы командной строки
    QApplication app(argc, argv);

    // Задаём название, версию и имя организации для приложения
    QCoreApplication::setApplicationName(QStringLiteral("LocalTextSimilarity"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("CourseworkProject"));

    // Создаём главное окно программы и отображаем его
    MainWindow window;
    window.show();

    // Запускаем цикл обработки событий приложения
    return app.exec();
}