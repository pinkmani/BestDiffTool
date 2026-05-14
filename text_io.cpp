#include "text_io.h"

#include <QFile>
#include <QStringConverter>
#include <QTextStream>

ReadTextFileResult readTextFile(const QString &path)
{
    ReadTextFileResult r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        r.error = QStringLiteral("Не удалось открыть: %1").arg(path);
        return r;
    }

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QByteArray utf8 = line.toUtf8();
        r.lines.emplace_back(utf8.constData(), static_cast<std::size_t>(utf8.size()));
    }

    // Файл, заканчивающийся переводом строки, даёт лишнюю пустую строку — убираем только финальную.
    while (!r.lines.empty() && r.lines.back().empty()) {
        r.lines.pop_back();
    }

    r.ok = true;
    return r;
}
