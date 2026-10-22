#include "main_window.h"

#include <QApplication>
#include <QFont>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Локальный антиплагиат"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("Coursework"));

    if (QStyle *style = QStyleFactory::create(QStringLiteral("Fusion"))) {
        QApplication::setStyle(style);
    }

    QFont appFont = app.font();
    appFont.setPointSize(10);
    app.setFont(appFont);

    MainWindow window;
    window.show();
    return app.exec();
}
