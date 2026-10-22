
#include "main_window.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("LocalTextSimilarity"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("CourseworkProject"));

    MainWindow window;
    window.show();

    return app.exec();
}
