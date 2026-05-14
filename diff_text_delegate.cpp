#include "diff_text_delegate.h"
#include "intraline_diff.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QTableView>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>

static void setupDiffHtmlDocument(QTextDocument &doc, const QString &htmlFragment, const QFont &font, int textWidth)
{
    doc.clear();
    doc.setDefaultFont(font);
    doc.setDocumentMargin(0);
    doc.setIndentWidth(0);
    doc.setHtml(QStringLiteral("<div style=\"margin:0;padding:0;line-height:1.0;white-space:pre;\">%1</div>")
                    .arg(htmlFragment));
    doc.setTextWidth(textWidth);

    QTextFrameFormat ff = doc.rootFrame()->frameFormat();
    ff.setMargin(0);
    ff.setPadding(0);
    ff.setBorder(0);
    doc.rootFrame()->setFrameFormat(ff);

    for (QTextBlock b = doc.firstBlock(); b.isValid(); b = b.next()) {
        QTextCursor c(b);
        QTextBlockFormat bf = b.blockFormat();
        bf.setTopMargin(0);
        bf.setBottomMargin(0);
        c.setBlockFormat(bf);
    }
}

void DiffTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    const QString html = index.data(kDiffLineHtmlRole).toString();
    if (html.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text.clear();
    opt.icon = QIcon();

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);

    QTextDocument doc;
    setupDiffHtmlDocument(doc, html, opt.font, textRect.width());

    painter->save();
    painter->translate(textRect.topLeft());
    doc.drawContents(painter);
    painter->restore();
}

QSize DiffTextDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    int w = opt.rect.width();
    if (w <= 0) {
        const auto *tv = qobject_cast<const QTableView *>(opt.widget);
        if (tv) {
            w = tv->columnWidth(index.column());
        }
        if (w <= 0) {
            w = 300;
        }
    }

    const int h = QFontMetrics(opt.font).height() + 6;
    return QSize(w, h);
}
